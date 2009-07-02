/**
 * @file llfloatermute.cpp
 * @brief Container for mute list
 *
 * @author Dale Glass <dale@daleglass.net>, (C) 2007
 */

#include "llviewerprecompiledheaders.h" // must be first include

#include "llavatarconstants.h"
#include "llfloateravatarlist.h"

#include "lluictrlfactory.h" // builds floaters from XML
#include "llviewerwindow.h"
#include "llscrolllistctrl.h"

#include "llvoavatar.h"
#include "llimview.h"
#include "llfloateravatarinfo.h"
#include "llregionflags.h"
#include "llfloaterreporter.h"
#include "llagent.h"
#include "llviewerregion.h"
#include "lltracker.h"
#include "llviewercontrol.h"
#include "llviewerstats.h"
#include "llerror.h"
#include "llchat.h"
#include "llviewermessage.h"
#include "llweb.h"
#include "llviewerobjectlist.h"
#include "llmutelist.h"
#include "llviewerimagelist.h"
#include "llworld.h"
#include "llcachename.h"
#include "llviewercamera.h"

#include <time.h>
#include <string.h>

#include <map>


// Timeouts
/**
 * @brief How long to keep showing an activity, in seconds
 */
const F32 ACTIVITY_TIMEOUT = 1.0f;


/**
 * @brief How many seconds to wait between data requests
 *
 * This is intended to avoid flooding the server with requests
 */
const F32 MIN_REQUEST_INTERVAL   = 1.0f;

/**
 * @brief How long to wait for a request to arrive during the first try in seconds
 */
const F32 FIRST_REQUEST_TIMEOUT  = 16.0f;

/**
 * @brief Delay is doubled on each attempt. This is as high as it'll go
 */
const F32 MAX_REQUEST_TIMEOUT    = 2048.0f;

/**
 * How long to wait for a request to arrive before assuming failure
 * and showing the failure icon in the list. This is just for the user's
 * information, if a reply arrives after this interval we'll accept it anyway.
 */
const F32 REQUEST_FAIL_TIMEOUT   = 15.0f;

/**
 * How long to keep people who are gone in the list. After this time is reached,
 * they're not shown in the list anymore, but still kept in memory until
 * CLEANUP_TIMEOUT is reached.
 */
const F32 DEAD_KEEP_TIME = 10.0f;

/**
 * @brief How long to keep entries around before removing them.
 *
 * @note Longer term, data like birth and payment info should be cached on disk.
 */
const F32 CLEANUP_TIMEOUT = 3600.0f;


/**
 * @brief TrustNet channel
 * This is fixed in the adapter script.
 */
const S32 TRUSTNET_CHANNEL = 0x44470002;


extern U32 gFrameCount;


LLAvListTrustNetScore::LLAvListTrustNetScore(std::string type, F32 score)
{
	Score = score;
	Type = type;
}

LLAvatarInfo::LLAvatarInfo()
{
}

LLAvatarInfo::LLAvatarInfo(PAYMENT_TYPE payment, ACCOUNT_TYPE account, struct tm birth)
{
	Payment = payment;
	Account = account;
	BirthDate = birth;
}

S32 LLAvatarInfo::getAge()
{
	time_t birth = mktime(&BirthDate);
	time_t now = time(NULL);
	return(S32)(difftime(now,birth) / (60*60*24));
}

void LLAvatarListEntry::setPosition(LLVector3d position)
{
	if ( mPosition != position )
	{
		setActivity(ACTIVITY_MOVING);
	}

	mPosition = position;
	mFrame = gFrameCount;
	mUpdateTimer.start();
}

LLVector3d LLAvatarListEntry::getPosition()
{
	return mPosition;
}

U32 LLAvatarListEntry::getEntryAgeFrames()
{
	return (gFrameCount - mFrame);
}

F32 LLAvatarListEntry::getEntryEnteredSeconds()
{
	return mEnteredTimer.getElapsedTimeF32();
}

F32 LLAvatarListEntry::getEntryAgeSeconds()
{
	return mUpdateTimer.getElapsedTimeF32();
}

void LLAvatarListEntry::setName(std::string name)
{
	if ( name.empty() || (name.compare(" ") == 0))
	{
		llwarns << "Trying to set empty name" << llendl;
	}
	mName = name;
}

std::string LLAvatarListEntry::getName()
{
	return mName;
}

LLUUID LLAvatarListEntry::getID()
{
	return mID;
}

void LLAvatarListEntry::setID(LLUUID id)
{
	if ( id.isNull() )
	{
		llwarns << "Trying to set null id" << llendl;
	}
	mID = id;
}

BOOL LLAvatarListEntry::getIsLinden()
{
	// Are there any employees that are not a Linden?
	// I suppose this is a bit redundant.
	return ( mIsLinden || ( mAvatarInfo.getValue().Account == ACCOUNT_EMPLOYEE ) );
}

void LLAvatarListEntry::setAccountCustomTitle(std::string &title)
{
	mAccountTitle = title;
	mAvatarInfo.getValue().Account = ACCOUNT_CUSTOM;
}

std::string LLAvatarListEntry::getAccountCustomTitle()
{
	return mAccountTitle;
}



void LLAvatarListEntry::setActivity(ACTIVITY_TYPE activity)
{
	if ( activity >= mActivityType || mActivityTimer.getElapsedTimeF32() > ACTIVITY_TIMEOUT )
	{
		mActivityType = activity;
		mActivityTimer.start();
	}
}

ACTIVITY_TYPE LLAvatarListEntry::getActivity()
{
	if ( mActivityTimer.getElapsedTimeF32() > ACTIVITY_TIMEOUT )
	{
		mActivityType = ACTIVITY_NONE;
	}

	return mActivityType;
}

void LLAvatarListEntry::toggleMark()
{
	mMarked = !mMarked;
}

BOOL LLAvatarListEntry::isMarked()
{
	return mMarked;
}

BOOL LLAvatarListEntry::isDead()
{
	return getEntryAgeSeconds() > DEAD_KEEP_TIME;
}

// Avatar list is global
LLFloaterAvatarList* gFloaterAvatarList = NULL;




LLFloaterAvatarList::LLFloaterAvatarList() :  LLFloater("avatar list")
{

	// Default values
	mTracking = FALSE;
	mTrackByLocation = FALSE;
	mARLastFrame = 0;

	// Create interface from XML
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_avatar_scanner.xml");

	// Floater starts hidden
	setVisible(FALSE);

	// Set callbacks
	//childSetAction("refresh_btn", onClickRefresh, this);
	childSetAction("profile_btn", onClickProfile, this);
	childSetAction("im_btn", onClickIM, this);
	childSetAction("track_btn", onClickTrack, this);
	childSetAction("mark_btn", onClickMark, this);

	childSetAction("gowarn_btn", onClickGohomerWarn, this);
	childSetAction("goeject_btn", onClickGohomerEject, this);
	childSetAction("goaway_btn", onClickGohomerSendAway, this);
	childSetAction("gohome_btn", onClickGohomerSendHome, this);
	childSetAction("gohomeoff_btn", onClickGohomerOff, this);
	childSetAction("gokey_btn", onClickGohomerSendHomeByKey, this);

	childSetAction("prev_in_list_btn", onClickPrevInList, this);
	childSetAction("next_in_list_btn", onClickNextInList, this);
	childSetAction("prev_marked_btn", onClickPrevMarked, this);
	childSetAction("next_marked_btn", onClickNextMarked, this);

	childSetAction("get_key_btn", onClickGetKey, this);

	childSetAction("tn_rate_btn", onClickTrustNetRate, this);
	childSetAction("tn_explain_btn", onClickTrustNetExplain, this);
	childSetAction("tn_website_btn", onClickTrustNetWebsite, this);
	childSetAction("tn_password_btn", onClickTrustNetGetPassword, this);
	childSetAction("tn_renew_btn", onClickTrustNetRenew, this);

	childSetAction("freeze_btn", onClickFreeze, this);
	childSetAction("eject_btn", onClickEject, this);
//	childSetAction("ban_btn", onClickBan, this);
//	childSetAction("unban_btn", onClickUnban, this);
	childSetAction("mute_btn", onClickMute, this);
//	childSetAction("unmute_btn", onClickUnmute, this);
	childSetAction("ar_btn", onClickAR, this);
	childSetAction("teleport_btn", onClickTeleport, this);
	childSetAction("estate_eject_btn", onClickEjectFromEstate, this);

	setDefaultBtn("refresh_btn");

	// Get a pointer to the scroll list from the interface
	mAvatarList = getChild<LLScrollListCtrl>("avatar_list");

	mAvatarList->setCallbackUserData(this);
	mAvatarList->setDoubleClickCallback(onDoubleClick);
	mAvatarList->sortByColumn("distance", TRUE);
	mDataRequestTimer.start();
	refreshAvatarList();

	LLMessageSystem *msg = gMessageSystem;
	msg->addHandlerFunc("AvatarPropertiesReply", processAvatarPropertiesReply);
}

LLFloaterAvatarList::~LLFloaterAvatarList()
{
	LLMessageSystem *msg = gMessageSystem;
	if ( msg )
	{
		msg->delHandlerFunc("AvatarPropertiesReply", processAvatarPropertiesReply);
	}
	std::map< LLUUID, LLPointer< LLHUDObject > >::iterator it = mHudObjectMap.begin();
	for ( ; it != mHudObjectMap.end(); ++it )
	{ // clean up list
		it->second->markDead();
	}

}


void LLFloaterAvatarList::show()
{
	// Make sure we make a noise.
	open();
}

//static
void LLFloaterAvatarList::toggle(void*) {
	if (!gFloaterAvatarList) {
		llinfos << "No avatar list!" << llendl;
		return;
	}

	if (gFloaterAvatarList->getVisible())
	{
		gFloaterAvatarList->close();
	}
	else
	{
		gFloaterAvatarList->show();
	}
}

//static
BOOL LLFloaterAvatarList::visible(void*)
{
	return (gFloaterAvatarList && gFloaterAvatarList->getVisible());
}

void LLFloaterAvatarList::updateFromCoarse()
{
	/*
	 * Walk through remaining list of coarse update avatars in all known regions
	 * this will not give us an accurate height since it's mod 2048 and least possible
	 * increment is 4 meter. Coarse Update information is accurate instantly while
	 * the object list is filled one by one.
	 *
	 * This also works for neighbour sims which makes it really handy :)
	 */

	// first wipe the list clean from coarse entries
	std::map<LLUUID, LLAvatarListEntry>::iterator iter;
	for(iter = mAvatars.begin(); iter != mAvatars.end();)
	{
		LLAvatarListEntry entry = iter->second;
		if ( entry.getIsCoarse() )
		{
			mAvatars.erase( iter++ );
		}
		else
		{
			++iter;
		}
	}

	LLWorld::region_list_t regions = LLWorld::getInstance()->getRegionList();
	LLWorld::region_list_t::const_iterator it = regions.begin();

	for ( ; it != regions.end(); ++it )
	{
		LLViewerRegion const *region = *it;
		if ( !region )
		{
			llwarns << "null region while parsing region list" << llendl;
			continue;
		}

		for (int idx = 0; idx < region->mMapAvatarIDs.count(); ++idx)
		{
			LLUUID avid = region->mMapAvatarIDs.get( idx );

			if ( avid.isNull() )
			{
				continue;
			}

			// we need to accomodate for avatars that are stuck in the
			// object list while still accurately received in the coarse
			// location list
			U32 modpos = region->mMapAvatars.get( idx );
			LLVector3 localpos;
			localpos[0] = (modpos >> 16) & 0xff;
			localpos[1] = (modpos >>  8) & 0xff;
			// scale z-position
			localpos[2] = (modpos        & 0xff) << 2;
			LLVector3d position = region->getPosGlobalFromRegion( localpos );

			if ( ( mAvatars.count( avid ) > 0 ) && ( !mAvatars[ avid ].isDead() ) )
			{
				// Avatar already in list but could be one of these "perpetual motion" avatars
				// which would overlay the real coordinates, so we check for the distance disregarding
				// the z axis
				LLVector3d coarsepos = position;
				coarsepos[2]     = 0.0;
				LLVector3d vopos = mAvatars[ avid ].mPosition;
				vopos[2]         = 0.0;
				LLVector3d dist  = coarsepos - vopos;
				if ( dist.magVecSquared() > ( 50.0 * 50.0 ) )
				{
					// Avatar already in list, but position info is
					// out of sync so use coarse info, we can safely overwrite
					// the info here since we are called after the VOlist has
					// already been parsed. The only issue is that this will now
					// show the avatar as perpetually moving
					mAvatars[ avid ].setPosition( coarsepos );
				}
				// Avatar already in list, active and
				// close enough to coarse info, so skip
				continue;
			}

			// Avatar not there yet, add it
			std::string name;
			BOOL isLinden = FALSE;
			if ( !gCacheName->getFullName( avid, name ) )
			{
				continue; // wait for proper name
			}
			else
			{
				std::string first, last;
				gCacheName->getName( avid, first, last );
				if ( last == "Linden" )
				{
					isLinden = TRUE;
				}
			}

			std::string regionname;
			if ( gAgent.getRegion() && ( region->getName() != gAgent.getRegion()->getName() ) )
			{
				regionname = region->getName();
			}

			// add as coarse info
			LLAvatarListEntry entry(avid, name, position, isLinden, TRUE, regionname);
			mAvatars[avid] = entry;

			//llinfos << "avatar list refresh from coarse: adding " << name << llendl;

		}
	}
}


void LLFloaterAvatarList::purgeAvatarHUDMap()
{
	std::map< LLUUID, LLPointer< LLHUDObject > >::iterator huditer = mHudObjectMap.begin();
	while ( huditer != mHudObjectMap.end() )
	{
		if ( mAvatars.count( huditer->first ) == 0 )
		{
			huditer->second->markDead();
			mHudObjectMap.erase( huditer++ );
		}
		else
		{
			++huditer;
		}
	}
}


void LLFloaterAvatarList::updateAvatarList()
{
//	LLVOAvatar *avatarp;

	//llinfos << "avatar list refresh: updating map" << llendl;

	// Check whether updates are enabled
	LLCheckboxCtrl* check;
	check = getChild<LLCheckBoxCtrl>("update_enabled_cb");

	if ( !check->getValue() )
	{
		return;
	}


	/*
	 * Iterate over all the avatars known at the time
	 * NOTE: Is this the right way to do that? It does appear that LLVOAvatar::isInstances contains
	 * the list of avatars known to the client. This seems to do the task of tracking avatars without
	 * any additional requests.
	 *
	 * BUG: It looks like avatars sometimes get stuck in this list, and keep perpetually
	 * moving in the same direction. My current guess is that somewhere else the client
	 * doesn't notice an avatar disappeared, and keeps updating its position. This should
	 * be solved at the source of the problem.
	 */
	for (std::vector<LLCharacter*>::iterator iter = LLCharacter::sInstances.begin();
		iter != LLCharacter::sInstances.end(); ++iter)
	{
		LLVOAvatar* avatarp = (LLVOAvatar*) *iter;

		// Skip if avatar is dead(what's that?)
		// or if the avatar is ourselves.
		if (avatarp->isDead() || avatarp->isSelf())
		{
			continue;
		}

		// Get avatar data
		LLVector3d position = gAgent.getPosGlobalFromAgent(avatarp->getCharacterPosition());
		LLUUID avid = avatarp->getID();
		std::string name = avatarp->getFullname();

		// Apparently, sometimes the name comes out empty, with a " " name. This is because
		// getFullname concatenates first and last name with a " " in the middle.
		// This code will avoid adding a nameless entry to the list until it acquires a name.
		if (name.empty() || (name.compare(" ") == 0))
		{
			llinfos << "Name empty for avatar " << avid << llendl;
			continue;
		}

		if (avid.isNull())
		{
			llinfos << "Key empty for avatar " << name << llendl;
			continue;
		}

		if ( ( mAvatars.count( avid ) > 0 ) && !mAvatars[avid].getIsCoarse() )
		{
			// Avatar already in list, update position
			mAvatars[avid].setPosition(position);
		}
		else
		{
			// Avatar not there yet or only from coarse list, add it properly
			BOOL isLinden = ( std::string( avatarp->getNVPair("LastName")->getString() ) == "Linden" );

			LLAvatarListEntry entry(avid, name, position, isLinden);
			mAvatars[avid] = entry;

			sendAvatarPropertiesRequest(avid);
			llinfos << "avatar list refresh: adding " << name << llendl;

		}

	}

	updateFromCoarse();

//	llinfos << "avatar list refresh: done" << llendl;

	expireAvatarList();
	refreshAvatarList();

	purgeAvatarHUDMap();

	checkTrackingStatus();
	processARQueue();
}

void LLFloaterAvatarList::processARQueue()
{
	if ( mARQueue.empty() ) return;

	LLUUID avatar_id = mARQueue.front();

	if ( 0 == mARLastFrame )
	{
		// Start of the process: Move the camera to the avatar. This happens gradually,
		// so we'll give it a few frames
		gAgent.lookAtObject(avatar_id, CAMERA_POSITION_OBJECT);
		mARLastFrame = gFrameCount;
		return;
	}

	if ( gFrameCount - mARLastFrame >= 10 )
	{
		// Camera should be in position, show AR screen now
		LLFloaterReporter *report = LLFloaterReporter::showFromObject(avatar_id, false);
		report->setMinimized(TRUE);

		mARReporterQueue.push(report);

		mARQueue.pop();
		mARLastFrame = 0;

		if ( mARQueue.empty() )
		{
			// Now that all reports are taken, open them.

			while( !mARReporterQueue.empty() )
			{
				LLFloaterReporter *r = mARReporterQueue.front();
				mARReporterQueue.pop();

				r->open();
				r->setMinimized(FALSE);
			}
		}
	}
}

void LLFloaterAvatarList::expireAvatarList()
{
//	llinfos << "avatar list: expiring" << llendl;
	std::map<LLUUID, LLAvatarListEntry>::iterator iter;
	std::queue<LLUUID> delete_queue;

	for(iter = mAvatars.begin(); iter != mAvatars.end(); iter++)
	{
		LLAvatarListEntry *ent = &iter->second;

		if ( ent->getEntryAgeFrames() >= 2 )
		{
			ent->setActivity(ACTIVITY_DEAD);
		}


		if ( ent->getEntryAgeSeconds() > CLEANUP_TIMEOUT )
		{
			llinfos << "avatar list: expiring avatar " << ent->getName() << llendl;
			LLUUID av_id = ent->getID();
			delete_queue.push(av_id);
		}
	}

	while(!delete_queue.empty())
	{
		mAvatars.erase(delete_queue.front());
		if ( mHudObjectMap.count(delete_queue.front()) )
		{
			mHudObjectMap[delete_queue.front()]->markDead();
			mHudObjectMap.erase(delete_queue.front());
		}
		delete_queue.pop();
	}
}

/**
 * Redraws the avatar list
 * Only does anything if the avatar list is visible.
 * @author Dale Glass
 */
void LLFloaterAvatarList::refreshAvatarList()
{



	// Don't update list when interface is hidden
	if (!LLFloaterAvatarList::visible(NULL))
	{
		return;
	}


	LLCheckboxCtrl* fetch_data;
	fetch_data = getChild<LLCheckBoxCtrl>("fetch_avdata_enabled_cb");

	//BOOL db_enabled = gSavedSettings.getBOOL("DBEnabled");
	//std::string db_avatar = gSavedPerAccountSettings.getString("DBAvatarName");
	//if ( db_avatar.empty() )
	//{
	//	db_enabled = FALSE;
	//}



	// We rebuild the list fully each time it's refreshed

	// The assumption is that it's faster to refill it and sort than
	// to rebuild the whole list.
	LLDynamicArray<LLUUID> selected = mAvatarList->getSelectedIDs();
	S32 scrollpos = mAvatarList->getScrollPos();

	mAvatarList->deleteAllItems();

	LLVector3d mypos = gAgent.getPositionGlobal();

	unsigned int counter = 0;

	std::map<LLUUID, LLAvatarListEntry>::iterator iter;
	for(iter = mAvatars.begin(); iter != mAvatars.end(); iter++)
	{
		LLSD element;
		LLUUID av_id;


		LLAvatarListEntry *ent = &iter->second;

		// Skip if avatar hasn't been around
		if ( ent->isDead() )
		{
			continue;
		}

		av_id = ent->getID();

		// Get avatar name, position
		LLAvatarInfo avinfo = ent->mAvatarInfo.getValue();
		//LLAvListTrustNetScore avscore = ent->mTrustNetScore.getValue();

		DATA_STATUS avinfo_status = ent->mAvatarInfo.getStatus();
		//DATA_STATUS avscore_status = ent->mTrustNetScore.getStatus();

		LLVector3d position = ent->getPosition();
		LLVector3d delta = position - mypos;
		F32 distance = (F32)delta.magVec();

		std::string icon = "";

		// HACK: Workaround for an apparent bug:
		// sometimes avatar entries get stuck, and are registered
		// by the client as perpetually moving in the same direction.
		// this makes sure they get removed from the visible list eventually.
		// for the coarse list this is not necessary since it is always accurate
		if ( distance > 1024 && !ent->getIsCoarse() )
		{
			continue;
		}

		if ( av_id.isNull() )
		{
			llwarns << "Avatar with null key somehow got into the list!" << llendl;
			continue;
		}

		counter++;

		element["id"] = av_id;

		element["columns"][LIST_AVATAR_ICON]["column"] = "avatar_icon";
		element["columns"][LIST_AVATAR_ICON]["type"] = "text";
		if ( !ent->isMarked() )
		{ // show counter if not marked
			element["columns"][LIST_AVATAR_ICON]["value"] = llformat("%d", counter);
		}
		else
		{
			element["columns"][LIST_AVATAR_ICON]["type"] = "icon";
			const LLUUID flag_blue("e39cbfe7-c4e7-3bad-5e5f-958082d55046");
			element["columns"][LIST_AVATAR_ICON]["value"] = flag_blue.asString();
		}


		if ( ent->getIsLinden() )
		{
			element["columns"][LIST_AVATAR_NAME]["font-style"] = "BOLD";
		}

		if ( ent->getIsCoarse() )
		{
			element["columns"][LIST_AVATAR_NAME]["color"] = LLColor4::grey4.getValue();
		}

		if ( ent->isFocused() )
		{
			element["columns"][LIST_AVATAR_NAME]["color"] = LLColor4::cyan.getValue();
		}

		//element["columns"][LIST_AVATAR_NAME]["font-color"] = getAvatarColor(ent, distance).getValue();
		element["columns"][LIST_AVATAR_NAME]["column"] = "avatar_name";
		element["columns"][LIST_AVATAR_NAME]["type"] = "text";
		std::string agentname = ent->getName();
		if ( !ent->getIsSameRegion() )
		{
			agentname += " (" + ent->getRegionName() + ")";
		}
		element["columns"][LIST_AVATAR_NAME]["value"] = agentname.c_str();

		char temp[32];
		snprintf(temp, sizeof(temp), "%.2f", distance);

		element["columns"][LIST_DISTANCE]["column"] = "distance";
		element["columns"][LIST_DISTANCE]["type"] = "text";
		element["columns"][LIST_DISTANCE]["value"] = temp;
		element["columns"][LIST_DISTANCE]["color"] = getAvatarColor(ent, distance, CT_DISTANCE).getValue();


		if ( avinfo_status == DATA_RETRIEVED )
		{
			element["columns"][LIST_AGE]["column"] = "age";
			element["columns"][LIST_AGE]["type"] = "text";
			element["columns"][LIST_AGE]["value"] = avinfo.getAge();
			element["columns"][LIST_AGE]["color"] = getAvatarColor(ent, distance, CT_AGE).getValue();
		}

		const LLUUID info_error("bbda234c-c76e-8617-0a32-46cc15c5ec42");
		const LLUUID info_fetching("1468fae4-2f47-6e75-d39f-3ccbd443d31c");
		const LLUUID info_unknown("0f2d532a-1fc8-01bb-eed3-ef60e7943d1e");
		const LLUUID payment_info_charter("07bef5d9-31b2-4cc5-999e-c2cd8b5d3a69");
		const LLUUID payment_info_filled("9d61c4d5-e8f6-78ec-a64f-490e3a4c03d5");
		const LLUUID payment_info_used("49ac7ef9-caaa-750a-6ec1-51358f0a1672");

		/*
		element["columns"][LIST_SCORE]["column"] = "score";
		element["columns"][LIST_SCORE]["type"] = "text";

		icon = "";
		switch(avscore_status)
		{
			case DATA_UNKNOWN:
				icon = info_unknown.asString();
				break;
			case DATA_REQUESTING:
				icon = info_fetching.asString();
				break;
			case DATA_ERROR:
				icon =  info_error.asString();
			case DATA_RETRIEVED:
				element["columns"][LIST_SCORE]["value"] = avscore.Score;
				element["columns"][LIST_SCORE]["color"] = getAvatarColor(ent, distance, CT_SCORE).getValue();
				break;
		}

		if (!icon.empty() )
		{
			element["columns"][LIST_SCORE].erase("color");
			element["columns"][LIST_SCORE]["type"] = "icon";
			element["columns"][LIST_SCORE]["value"] = icon;
		}*/


		// Get an icon for the payment data
		// These should be replaced with something proper instead of reusing whatever
		// LL-provided images happened to fit
		icon = "";

		switch(avinfo_status)
		{
			case DATA_UNKNOWN:
				icon = info_unknown.asString();
				break;
			case DATA_REQUESTING:
				icon = info_fetching.asString();
				break;
			case DATA_ERROR:
				icon = info_error.asString();
				break;
			case DATA_RETRIEVED:
				switch(avinfo.Payment)
				{
					case PAYMENT_NONE:
						break;
					case PAYMENT_ON_FILE:
						icon =  payment_info_filled.asString();
						break;
					case PAYMENT_USED:
						icon =  payment_info_used.asString();
						break;
					case PAYMENT_LINDEN:
						// confusingly named icon, maybe use something else
						icon =  "icon_top_pick.tga";
						break;
				}
				break;
		}

		element["columns"][LIST_PAYMENT]["column"] = "payment_data";
		element["columns"][LIST_PAYMENT]["type"] = "text";

		// TODO: Add icon for "unknown" status
		//if ( PAYMENT_NONE != avinfo.Payment && DATA_UNKNOWN != avinfo_status )
		if ( !icon.empty() )
		{
			element["columns"][LIST_PAYMENT].erase("color");
			element["columns"][LIST_PAYMENT]["type"] = "icon";
			element["columns"][LIST_PAYMENT]["value"] =  icon;
			//llinfos << "Payment icon: " << payment_icon << llendl;
		}

		const LLUUID avatar_gone("db4592d5-c8a5-9336-019c-fcbd282d5f33");
		const LLUUID avatar_new("33d4b23e-a29c-ac03-f7f6-c2fa197b13fe");
		const LLUUID avatar_typing("6f083c3c-1e88-d184-6add-95402b3e108f");
		/*<avatar_sound.tga value = "439836e2-29f5-c12f-71d4-aa59283296e1"/>
		<flag_blue.tga value="e39cbfe7-c4e7-3bad-5e5f-958082d55046"/>
		<flag_green.tga value="78952758-1bef-f968-d382-b39094f85aa1"/>
		<flag_orange.tga value="c72ca7d9-42cd-02f1-ce32-ca1ea5d1c25d"/>
		<flag_pink.tga value="a3419a89-b8d9-293c-693e-12982e574304"/>
		<flag_purple.tga value="7982fbf8-457a-77ce-61e6-b3c7d9500d2f"/>
		<flag_red.tga value="11ba32bf-44fe-666e-073b-00768785b4d0"/>
		<flag_yellow.tga value="98a5a29e-e933-eeed-bdd9-4d461f557d34"/>*/

		ACTIVITY_TYPE activity = ent->getActivity();
		icon = "";
		switch( activity )
		{
			case ACTIVITY_NONE:
				break;
			case ACTIVITY_MOVING:
				icon = "inv_item_animation.tga";
				break;
			case ACTIVITY_GESTURING:
				icon = "inv_item_gesture.tga";
				break;
			case ACTIVITY_SOUND:
				icon = "inv_item_sound.tga";
				break;
			case ACTIVITY_REZZING:
				icon = "ff_edit_theirs.tga";
				break;
			case ACTIVITY_PARTICLES:
				// TODO: Replace with something better
				icon = "account_id_green.tga";
				break;
			case ACTIVITY_NEW:
				icon = avatar_new.asString();
				break;
			case ACTIVITY_TYPING:
				icon = avatar_typing.asString();
				break;
			case ACTIVITY_DEAD:
				// TODO: Replace, icon is quite inappropiate
				icon = avatar_gone.asString();
				break;
		}

		element["columns"][LIST_ACTIVITY]["column"] = "activity";
		element["columns"][LIST_ACTIVITY]["type"] = "text";

		if (!icon.empty() )
		{
			element["columns"][LIST_ACTIVITY]["type"] = "icon";
			element["columns"][LIST_ACTIVITY]["value"] = icon;
			//llinfos << "Activity icon: " << activity_icon << llendl;
		}

		char tempentered[32];
		F32 entered = ent->getEntryEnteredSeconds();
		snprintf(tempentered, sizeof(tempentered), "%u", (unsigned int)(entered/60.0));
		element["columns"][LIST_ENTERED]["column"] = "entered";
		element["columns"][LIST_ENTERED]["type"] = "text";
		element["columns"][LIST_ENTERED]["value"] = tempentered;

		
		element["columns"][LIST_CLIENT]["column"] = "client";
		element["columns"][LIST_CLIENT]["type"] = "text";
		LLColor4 avatar_name_color = gColors.getColor( "AvatarNameColor" );
		std::string client;
		LLVOAvatar *av = (LLVOAvatar*)gObjectList.findObject(av_id);
		if(av)
		{
			LLVOAvatar::resolveClient(avatar_name_color, client, av);
			if(client == "")
			{
				avatar_name_color = gColors.getColor( "ScrollUnselectedColor" );
				client = "?";
			}
			element["columns"][LIST_CLIENT]["value"] = client.c_str();
			//element["columns"][LIST_CLIENT]["color"] = avatar_name_color.getValue();
		}else
		{
			element["columns"][LIST_CLIENT]["value"] = "Out Of Range";
		}
		element["columns"][LIST_CLIENT]["color"] = avatar_name_color.getValue();

		// Add to list
		mAvatarList->addElement(element, ADD_BOTTOM);

		// Request data only if fetching avatar data is enabled
		if ( fetch_data->getValue() && ent->mAvatarInfo.requestIfNeeded() )
		{
			sendAvatarPropertiesRequest(av_id);
			llinfos << "Data for avatar " << ent->getName() << " didn't arrive yet, retrying" << llendl;
		}

		/*if ( ent->mTrustNetScore.requestIfNeeded() )
		{
			requestTrustNetScore(av_id, ent->getName(), "behavior");
			llinfos << "Requesting TrustNet score for " << ent->getName() << llendl;
		}*/

		//if ( db_enabled && ent->mMiscInfo.requestIfNeeded() )
		//{
		//	requestMiscInfo(av_id, ent->getName());
		//	llinfos << "Requesting misc info for " << ent->getName() << llendl;
		//}
	}

	// finish
	mAvatarList->sort();
	mAvatarList->selectMultiple(selected);
	mAvatarList->setScrollPos(scrollpos);

//	llinfos << "avatar list refresh: done" << llendl;

}

// static
void LLFloaterAvatarList::onClickIM(void* userdata)
{
	//llinfos << "LLFloaterFriends::onClickIM()" << llendl;
	LLFloaterAvatarList *avlist = (LLFloaterAvatarList*)userdata;

	LLDynamicArray<LLUUID> ids = avlist->mAvatarList->getSelectedIDs();
	if(ids.size() > 0)
	{
		if(ids.size() == 1)
		{
			// Single avatar
			LLUUID agent_id = ids[0];

			char buffer[MAX_STRING];
			snprintf(buffer, MAX_STRING, "%s", avlist->mAvatars[agent_id].getName().c_str());
			gIMMgr->setFloaterOpen(TRUE);
			gIMMgr->addSession(
				buffer,
				IM_NOTHING_SPECIAL,
				agent_id);
		}
		else
		{
			// Group IM
			LLUUID session_id;
			session_id.generate();
			gIMMgr->setFloaterOpen(TRUE);
			gIMMgr->addSession("Avatars Conference", IM_SESSION_CONFERENCE_START, ids[0], ids);
		}
	}
}

void LLFloaterAvatarList::onClickTrack(void *userdata)
{
	LLFloaterAvatarList *avlist = (LLFloaterAvatarList*)userdata;

 	LLScrollListItem *item =   avlist->mAvatarList->getFirstSelected();
	if (!item) return;

	LLUUID agent_id = item->getUUID();

	if ( avlist->mTracking && avlist->mTrackedAvatar == agent_id ) {
		LLTracker::stopTracking(NULL);
		avlist->mTracking = FALSE;
	}
	else
	{
		avlist->mTracking = TRUE;
		avlist->mTrackByLocation = FALSE;
		avlist->mTrackedAvatar = agent_id;
		LLTracker::trackAvatar(agent_id, avlist->mAvatars[agent_id].getName());
	}
}

void LLFloaterAvatarList::sendAvatarPropertiesRequest(LLUUID avid)
{


	lldebugs << "LLPanelAvatar::sendAvatarPropertiesRequest()" << llendl;
	LLMessageSystem *msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_AvatarPropertiesRequest);
	msg->nextBlockFast( _PREHASH_AgentData);
	msg->addUUIDFast(   _PREHASH_AgentID, gAgent.getID() );
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(   _PREHASH_AvatarID, avid);
	gAgent.sendReliableMessage();

	mAvatars[avid].mAvatarInfo.requestStarted();
}

// static
void LLFloaterAvatarList::processAvatarPropertiesReply(LLMessageSystem *msg, void**)
{


	LLFloaterAvatarList* self = NULL;
	LLAvatarInfo avinfo;

	BOOL	identified = FALSE;
	BOOL	transacted = FALSE;

	LLUUID	agent_id;	// your id
	LLUUID	avatar_id;	// target of this panel
	U32	flags = 0x0;
	char	born_on[DB_BORN_BUF_SIZE];
	S32	charter_member_size = 0;

	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AgentID, agent_id);
	msg->getUUIDFast(_PREHASH_AgentData, _PREHASH_AvatarID, avatar_id );


	self = gFloaterAvatarList;

	// Verify that the avatar is in the list, if not, ignore.
	if ( self->mAvatarList->getItemIndex(avatar_id) < 0 )
	{
		return;
	}

	LLAvatarListEntry *entry = &self->mAvatars[avatar_id];

	msg->getStringFast(_PREHASH_PropertiesData, _PREHASH_BornOn, DB_BORN_BUF_SIZE, born_on);
	msg->getU32Fast(_PREHASH_PropertiesData, _PREHASH_Flags, flags);

	identified = (flags & AVATAR_IDENTIFIED);
	transacted = (flags & AVATAR_TRANSACTED);

	// What's this?
	// Let's see if I understand correctly: CharterMember property is dual purpose:
	// it either contains a number indicating an account type (usual value), or
	// it contains a string with a custom title. Probably that's where Philip Linden's
	// "El Presidente" title comes from. Heh.
	U8 caption_index = 0;
	std::string caption_text;
	charter_member_size = msg->getSize("PropertiesData", "CharterMember");

	if(1 == charter_member_size)
	{
		msg->getBinaryData("PropertiesData", "CharterMember", &caption_index, 1);
	}
	else if(1 < charter_member_size)
	{
		char caption[MAX_STRING];
		msg->getString("PropertiesData", "CharterMember", MAX_STRING, caption);

		caption_text = caption;
		entry->setAccountCustomTitle(caption_text);
	}


	if(caption_text.empty())
	{

		const enum ACCOUNT_TYPE ACCT_TYPE[] = {
			ACCOUNT_RESIDENT,
			ACCOUNT_TRIAL,
			ACCOUNT_CHARTER_MEMBER,
			ACCOUNT_EMPLOYEE
		};

		//enum ACCOUNT_TYPE acct =
		avinfo.Account =  ACCT_TYPE[llclamp(caption_index, (U8)0, (U8)(sizeof(ACCT_TYPE)/sizeof(ACCT_TYPE[0])-1))];
		//entry->setAccountType(acct);


		if ( avinfo.Account != ACCOUNT_EMPLOYEE )
		{
			if ( transacted )
			{
				avinfo.Payment = PAYMENT_USED;
			}
			else if ( identified )
			{
				avinfo.Payment = PAYMENT_ON_FILE;
			}
			else
			{
				avinfo.Payment = PAYMENT_NONE;
			}
		}
		else
		{
			avinfo.Payment = PAYMENT_LINDEN;
		}
	}

	// Structure must be zeroed to have sane results, as we
	// have an incomplete string for input
	memset(&avinfo.BirthDate, 0, sizeof(avinfo.BirthDate));

	int num_read = sscanf(born_on, "%d/%d/%d", &avinfo.BirthDate.tm_mon,
	                                           &avinfo.BirthDate.tm_mday,
	                                           &avinfo.BirthDate.tm_year);

	if ( num_read == 3 && avinfo.BirthDate.tm_mon <= 12 )
	{
		avinfo.BirthDate.tm_year -= 1900;
		avinfo.BirthDate.tm_mon--;
	}
	else
	{
		// Zero again to remove any partially read data
		memset(&avinfo.BirthDate, 0, sizeof(avinfo.BirthDate));
		llwarns << "Error parsing birth date: " << born_on << llendl;
	}

	entry->mAvatarInfo.setValue(avinfo);
}

void LLFloaterAvatarList::checkTrackingStatus()
{

	if ( mTracking && LLTracker::getTrackedPositionGlobal().isExactlyZero() )
	{
		// trying to track an avatar, but tracker stopped tracking
		if ( mAvatars.count( mTrackedAvatar ) > 0 && !mTrackByLocation )
		{
			llinfos << "Switching to location-based tracking" << llendl;
			mTrackByLocation = TRUE;
		}
		else
		{
			// not found
			llinfos << "Stopping tracking avatar, server-side didn't work, and not in list anymore." << llendl;
			LLTracker::stopTracking(NULL);
			mTracking = FALSE;
		}
	}

	if ( mTracking && mTrackByLocation )
	{
		std::string name = mAvatars[mTrackedAvatar].getName();
		std::string tooltip = "Tracking last known position";
		name += " (near)";
		LLTracker::trackLocation(mAvatars[mTrackedAvatar].getPosition(), name, tooltip);
	}

	//llinfos << "Tracking position: " << LLTracker::getTrackedPositionGlobal() << llendl;

}


BOOL  LLFloaterAvatarList::avatarIsInList(LLUUID avatar)
{
	return ( mAvatars.count( avatar ) > 0 );
}

LLAvatarListEntry * LLFloaterAvatarList::getAvatarEntry(LLUUID avatar)
{
	if ( avatar.isNull() )
	{
		return NULL;
	}

	std::map<LLUUID, LLAvatarListEntry>::iterator iter;

	iter = mAvatars.find(avatar);
	if ( iter == mAvatars.end() )
	{
		return NULL;
	}

	return &iter->second;

	//if ( mAvatars.count( avatar ) < 0 )
	//{
		//return NULL;
	//}

	//return &mAvatars[avatar];
}

void LLFloaterAvatarList::speakText(S32 channel, EChatType type, std::string text)
{
	LLMessageSystem* msg = gMessageSystem;

	msg->newMessageFast(_PREHASH_ChatFromViewer);
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->nextBlockFast(_PREHASH_ChatData);
	msg->addStringFast(_PREHASH_Message, text);
	msg->addU8Fast(_PREHASH_Type, type);
	msg->addS32("Channel", channel);

	gAgent.sendReliableMessage();

	LLViewerStats::getInstance()->incStat(LLViewerStats::ST_CHAT_COUNT);
}


void LLFloaterAvatarList::requestTrustNetScore(LLUUID avatar, const std::string name, const std::string type)
{
	char *temp = new char[UUID_STR_LENGTH];
	avatar.toString(temp);

	std::string text = "GetScore|" + name + "|" + temp + "|" + type;
	speakText(TRUSTNET_CHANNEL, CHAT_TYPE_WHISPER, text);
}

//static
void LLFloaterAvatarList::replaceVars(std::string &str, LLUUID avatar, const std::string& name)
{
	char *temp = new char[UUID_STR_LENGTH];
	avatar.toString(temp);

	std::string vars[][2] = {
		{"$NAME", name},
		{"$KEY",  temp},
	};

	BOOL replaced = TRUE;

	while( replaced )
	{
		replaced = FALSE;
		for(U32 i=0;i<sizeof(vars)/sizeof(vars[0]);i++)
		{
			std::string::size_type pos = str.find(vars[i][0]);
			if ( pos != std::string::npos )
			{
				str.replace(pos, vars[i][0].size(), vars[i][1]);
				replaced = TRUE;
			}
		}
	}

}

void LLFloaterAvatarList::requestMiscInfo(LLUUID avatar, const std::string name)
{
	//LLUUID   db_av_key;

	//std::string message      = gSavedPerAccountSettings.getString("DBSendPattern");
	//std::string db_av_name   = gSavedPerAccountSettings.getString("DBAvatarName");
	//db_av_key.set(gSavedPerAccountSettings.getString("DBAvatarKey"));


	//llinfos << "Requesting info " << llendl;
	//replaceVars(message, avatar, name);

	//llinfos << "Request string: " << message << llendl;
	//send_simple_im(db_av_key, message.c_str());
 }

//static
BOOL LLFloaterAvatarList::handleIM(LLUUID from_id, const std::string message)
{
	LLUUID   db_av_key;
	//db_av_key.set(gSavedPerAccountSettings.getString("DBAvatarKey"));

	if ( db_av_key == from_id )
	{
		std::map<LLUUID, LLAvatarListEntry>::iterator iter;

		for(iter = gFloaterAvatarList->mAvatars.begin(); iter != gFloaterAvatarList->mAvatars.end(); iter++)
		{
			LLAvatarListEntry *ent = &iter->second;

			// Check if the key, or the name are found in the reply.
			// Name is only accepted if it's in the beginning of the message.
			if ( message.find(ent->getID().asString()) != std::string::npos
			     || message.find(ent->getName().c_str()) == 0 )
			{
				LLMiscDBInfo info;
				info.data = message;

				llinfos << "Database reply arrived for avatar " << ent->getName() << llendl;
				ent->mMiscInfo.setValue(info);
			}
		}

		return TRUE;
	}
	return FALSE;
}

//static
void LLFloaterAvatarList::processTrustNetReply(char *reply)
{
	char *tokens[10];
	char *tmp = &reply[0];
	U32 count = 0;

	llinfos << "TrustNet reply: " << reply << llendl;


	// Split into tokens
	while( (NULL != (tmp = strtok(tmp, "|"))) && count < (sizeof(tokens)/sizeof(tokens[0])) )
	{
		tokens[count++] = tmp;
		llinfos << "token: " << tmp << llendl;
		tmp = NULL;
	}

	llinfos << "Got " << count << " tokens" << llendl;

	if ( count >= 1 )
	{
		if (!strcmp(tokens[0], "Score") && count >= 4)
		{
			//format: key|type|score
			LLUUID avatar(tokens[1]);
			std::string type = tokens[2];
			F32 score = (F32)strtod(tokens[3], NULL);

			LLAvatarListEntry *ent = gFloaterAvatarList->getAvatarEntry(avatar);
			if ( ent != NULL )
			{
				LLAvListTrustNetScore s(type, score);
				ent->mTrustNetScore.setValue(s);
				llinfos << "Score arrived for avatar " << avatar << ": " << score << llendl;
			}
			else
			{
				llinfos << "Score arrived for avatar " << avatar << ", but it wasn't in the list anymore" << llendl;
			}
		}
		else if (!strcmp(tokens[0], "WebAuthToken") && count >= 2)
		{
			std::string URL = LLWeb::escapeURL(llformat("http://trustnet.daleglass.net/?session=%s", tokens[1]));
 			LLWeb::loadURL(URL);
		}
		else if (!strcmp(tokens[0], "WebPassword") && count >= 2)
		{
			std::string password = tokens[1];
			gViewerWindow->mWindow->copyTextToClipboard(utf8str_to_wstring(password));
		}
		else
		{
			llwarns << "Unrecognized TrustNet reply " << tokens[0] << llendl;
		}
	}
}

void LLFloaterAvatarList::luskwoodCommand(std::string cmd)
{
	LLDynamicArray<LLUUID> ids = mAvatarList->getSelectedIDs();

	for(LLDynamicArray<LLUUID>::iterator itr = ids.begin(); itr != ids.end(); ++itr)
	{
		LLUUID avid = *itr;
		LLAvatarListEntry *ent = getAvatarEntry(avid);
		if ( ent != NULL )
		{
			//llinfos << "Would say: " << cmd << " " << ent->getName() << llendl;
			// Use key got gokey, name for everything else
			speakText(0, CHAT_TYPE_SHOUT, cmd + " " + ( cmd == "gokey" ? ent->getID().asString() :  ent->getName() ) );
		}
	}
}

//static
void LLFloaterAvatarList::onClickMark(void *userdata)
{
	LLFloaterAvatarList *avlist = (LLFloaterAvatarList*)userdata;
	LLDynamicArray<LLUUID> ids = avlist->mAvatarList->getSelectedIDs();

	for(LLDynamicArray<LLUUID>::iterator itr = ids.begin(); itr != ids.end(); ++itr)
	{
		LLUUID avid = *itr;
		LLAvatarListEntry *ent = avlist->getAvatarEntry(avid);
		if ( ent != NULL )
		{
			ent->toggleMark();
		}
	}
}

void LLFloaterAvatarList::handleLuskwoodDialog(S32 option, void* data)
{
	LLFloaterAvatarList *self = (LLFloaterAvatarList*)data;
	if ( 0 == option )
	{
		self->luskwoodCommand(self->mLuskwoodCommand);
	}
}

void LLFloaterAvatarList::handleLuskwoodGohomerOffDialog(S32 option, void* data)
{
	LLFloaterAvatarList *self = (LLFloaterAvatarList*)data;
	if ( 0 == option )
	{
		self->speakText(0, CHAT_TYPE_SHOUT, "gohome off");
	}
}

//static
void LLFloaterAvatarList::onClickGohomerWarn(void *data)
{
	LLFloaterAvatarList *self = (LLFloaterAvatarList*)data;

	self->mLuskwoodCommand = "gowarn";
	gViewerWindow->alertXml("LuskwoodGohomerWarn", handleLuskwoodDialog, self);

}

//static
void LLFloaterAvatarList::onClickGohomerEject(void *data)
{
	LLFloaterAvatarList *self = (LLFloaterAvatarList*)data;

	self->mLuskwoodCommand = "goeject";
	gViewerWindow->alertXml("LuskwoodGohomerEject", handleLuskwoodDialog, self);
}

//static
void LLFloaterAvatarList::onClickGohomerSendAway(void *data)
{
	LLFloaterAvatarList *self = (LLFloaterAvatarList*)data;

	self->mLuskwoodCommand = "goaway";
	gViewerWindow->alertXml("LuskwoodGohomerKeepAway", handleLuskwoodDialog, self);
}

//static
void LLFloaterAvatarList::onClickGohomerSendHome(void *data)
{
	LLFloaterAvatarList *self = (LLFloaterAvatarList*)data;

	self->mLuskwoodCommand = "gohome";
	gViewerWindow->alertXml("LuskwoodGohomerSendHome", handleLuskwoodDialog, self);
}

//static
void LLFloaterAvatarList::onClickGohomerSendHomeByKey(void *data)
{
	LLFloaterAvatarList *self = (LLFloaterAvatarList*)data;

	self->mLuskwoodCommand = "gokey";
	gViewerWindow->alertXml("LuskwoodGohomerSendHome", handleLuskwoodDialog, self);
}


//static
void LLFloaterAvatarList::onClickGohomerOff(void *data)
{
	LLFloaterAvatarList *self = (LLFloaterAvatarList*)data;

	gViewerWindow->alertXml("LuskwoodGohomerOff", handleLuskwoodGohomerOffDialog, self);
}

LLColor4 LLFloaterAvatarList::getAvatarColor(LLAvatarListEntry *ent, F32 distance, e_coloring_type type)
{
 	F32 r = 0.0f, g = 0.0f, b = 0.0f, a = 1.0f;

	switch(type)
	{
		case CT_NONE:
			return LLColor4::black;
			break;
		case CT_DISTANCE:
			if ( distance <= 10.0f )
			{
				// whisper range
				g = 0.7f - ( distance / 20.0f );
			}
			else if ( distance > 10.0f && distance <= 20.0f )
			{
				// talk range
				g = 0.7f - ( (distance - 10.0f) / 20.0f );
				b = g;
			}
			else if ( distance > 20.0f && distance <= 96.0f )
			{
				// shout range
				r = 0.7f - ( (distance - 20.0f) / 192.0f );
				b = r;
			}
			else
			{
				// unreachable by chat
				r = 1.0;
			}
			break;
		case CT_AGE:
			if ( ent->mAvatarInfo.getStatus() == DATA_RETRIEVED )
			{
				S32 age = ent->mAvatarInfo.getValue().getAge();
				if ( age < 14 )
				{
					r = 0.7f - ( age / 28 );
				}
				else if ( age > 14 && age <= 30 )
				{
					r = 0.7f - ( (age-14) / 32 );
					g = r;
				}
				else if ( age > 30 && age < 90 )
				{
					g = 0.7f - ( (age-30) / 120 );
				}
				else
				{
					b = 1.0f;
				}
			}
			break;
		case CT_SCORE:
			if ( ent->mTrustNetScore.getStatus() == DATA_RETRIEVED )
			{
				F32 score = ent->mTrustNetScore.getValue().Score;

				if ( score == 0.0 )
				{
					b = 1.0f;
				}
				else if ( score == 10.0f )
				{
					g = 1.0f;
				}
				else if ( score == -10.0f )
				{
					r = 1.0f;
				}
				else if ( score > 0.0f )
				{
					g = 0.2f + ( score / 20.0f );
				}
				else if ( score < 0.0f )
				{
					r = 0.2f + ( score / 20.0f );
				}
			}
			break;
		case CT_PAYMENT:
			break;
		case CT_ENTERED:
			F32 entered = ent->getEntryEnteredSeconds();
			if (distance <= 20.0f)
			{
				if ( entered <= ( 5.0f * 60.0f ) )
				{
					r = 0.7f - ( entered / ( 4.0f * 5.0f * 60.0f ) );
					b = 1.0f - ( ( ( 5.0f * 60.0f ) - entered) / ( 5.0f * 60.0f ) );
				}
				else
				{
					b = 1.0f;
				}
			}
			else
			{
				r = g = b = 0.5f;
			}
			break;
	}

	return LLColor4(r,g,b,a);
}

void LLFloaterAvatarList::onDoubleClick(void *userdata)
{
	LLFloaterAvatarList *self = (LLFloaterAvatarList*)userdata;
 	LLScrollListItem *item =   self->mAvatarList->getFirstSelected();
	LLUUID agent_id = item->getUUID();
	LLAvatarListEntry *ent = 0;

	if ( self->mAvatars.count( agent_id ) )
	{
		ent = &self->mAvatars[ agent_id ];
	}

	if ( ent && ent->getIsCoarse() )
	{
		// nothing to look at, manipulate camera directly
		LLQuaternion rot;

		LLMatrix3 mat(rot);

		LLVector3 pos( ent->getPosition() );

		LLViewerCamera::getInstance()->setView(1.0);
		LLViewerCamera::getInstance()->setOrigin( pos );
		LLViewerCamera::getInstance()->mXAxis = LLVector3(mat.mMatrix[0]);
		LLViewerCamera::getInstance()->mYAxis = LLVector3(mat.mMatrix[1]);
		LLViewerCamera::getInstance()->mZAxis = LLVector3(mat.mMatrix[2]);	}
	else
	{
		gAgent.lookAtObject(agent_id, CAMERA_POSITION_OBJECT);
	}
}

void LLFloaterAvatarList::removeFocusFromAll()
{
	std::map<LLUUID, LLAvatarListEntry>::iterator iter;

	for(iter = mAvatars.begin(); iter != mAvatars.end(); iter++)
	{
		LLAvatarListEntry *ent = &iter->second;
		ent->setFocus(FALSE);
	}
}

void LLFloaterAvatarList::focusOnPrev(BOOL marked_only)
{
	std::map<LLUUID, LLAvatarListEntry>::iterator iter;
	LLAvatarListEntry *prev = NULL;
	LLAvatarListEntry *ent;

	if ( mAvatars.size() == 0 )
	{
		return;
	}

	for(iter = mAvatars.begin(); iter != mAvatars.end(); iter++)
	{
		ent = &iter->second;

		if ( ent->isDead() )
			continue;

		if ( (ent->getID() == mFocusedAvatar) && (prev != NULL)  )
		{
			removeFocusFromAll();
			prev->setFocus(TRUE);
			mFocusedAvatar = prev->getID();
			gAgent.lookAtObject(mFocusedAvatar, CAMERA_POSITION_OBJECT);
			return;
		}

		if ( (!marked_only) || ent->isMarked() )
		{
			prev = ent;
		}
	}

	if (prev != NULL && ((!marked_only) || prev->isMarked()) )
	{
		removeFocusFromAll();
		prev->setFocus(TRUE);
		mFocusedAvatar = prev->getID();
		gAgent.lookAtObject(mFocusedAvatar, CAMERA_POSITION_OBJECT);
	}
}

void LLFloaterAvatarList::focusOnNext(BOOL marked_only)
{


	std::map<LLUUID, LLAvatarListEntry>::iterator iter;
	BOOL found = FALSE;
	LLAvatarListEntry *first = NULL;
	LLAvatarListEntry *ent;

	if ( mAvatars.size() == 0 )
	{
		return;
	}

	for(iter = mAvatars.begin(); iter != mAvatars.end(); iter++)
	{
		ent = &iter->second;

		if ( ent->isDead() )
			continue;

		if ( NULL == first && ((!marked_only) || ent->isMarked()))
		{
			first = ent;
		}

		if ( found && ((!marked_only) || ent->isMarked()) )
		{
			removeFocusFromAll();
			ent->setFocus(TRUE);
			mFocusedAvatar = ent->getID();
			gAgent.lookAtObject(mFocusedAvatar, CAMERA_POSITION_OBJECT);
			return;
		}

		if ( ent->getID() == mFocusedAvatar )
		{
			found = TRUE;
		}
	}

	if (first != NULL && ((!marked_only) || first->isMarked()))
	{
		removeFocusFromAll();
		first->setFocus(TRUE);
		mFocusedAvatar = first->getID();
		gAgent.lookAtObject(mFocusedAvatar, CAMERA_POSITION_OBJECT);
	}
}
//static
void LLFloaterAvatarList::onClickPrevInList(void *userdata)
{
	LLFloaterAvatarList *self = (LLFloaterAvatarList*)userdata;
	self->focusOnPrev(FALSE);
}

//static
void LLFloaterAvatarList::onClickNextInList(void *userdata)
{
	LLFloaterAvatarList *self = (LLFloaterAvatarList*)userdata;
	self->focusOnNext(FALSE);
}

//static
void LLFloaterAvatarList::onClickPrevMarked(void *userdata)
{
	LLFloaterAvatarList *self = (LLFloaterAvatarList*)userdata;
	self->focusOnPrev(TRUE);
}

//static
void LLFloaterAvatarList::onClickNextMarked(void *userdata)
{
	LLFloaterAvatarList *self = (LLFloaterAvatarList*)userdata;
	self->focusOnNext(TRUE);
}

//static
void LLFloaterAvatarList::onClickTrustNetRate(void *userdata)
{
	// LLFloaterAvatarList *self = (LLFloaterAvatarList*)userdata;
	llinfos << "Ratings not implemented yet" << llendl;
}

//static
void LLFloaterAvatarList::onClickTrustNetExplain(void *userdata)
{
	LLFloaterAvatarList *self = (LLFloaterAvatarList*)userdata;
	LLScrollListItem *item =   self->mAvatarList->getFirstSelected();

	if ( item != NULL )
	{
		LLAvatarListEntry *ent = self->getAvatarEntry(item->getUUID());
		self->speakText(TRUSTNET_CHANNEL, CHAT_TYPE_WHISPER, "Explain|" + ent->getName() + "|" + ent->getID().asString());
	}
}

//static
void LLFloaterAvatarList::onClickTrustNetWebsite(void *userdata)
{
	LLFloaterAvatarList *self = (LLFloaterAvatarList*)userdata;

	self->speakText(TRUSTNET_CHANNEL, CHAT_TYPE_WHISPER, "GetWebAuthToken");
}

//static
void LLFloaterAvatarList::onClickTrustNetGetPassword(void *userdata)
{
	LLFloaterAvatarList *self = (LLFloaterAvatarList*)userdata;

	self->speakText(TRUSTNET_CHANNEL, CHAT_TYPE_WHISPER, "GetWebPassword");
}

//static
void LLFloaterAvatarList::onClickTrustNetRenew(void *userdata)
{
	LLFloaterAvatarList *self = (LLFloaterAvatarList*)userdata;
	self->speakText(TRUSTNET_CHANNEL, CHAT_TYPE_WHISPER, "RenewSubscription");
}

//static
void LLFloaterAvatarList::onClickGetKey(void *userdata)
{
	LLFloaterAvatarList *self = (LLFloaterAvatarList*)userdata;
 	LLScrollListItem *item = self->mAvatarList->getFirstSelected();

	if ( NULL == item ) return;

	LLUUID agent_id = item->getUUID();

	char buffer[UUID_STR_LENGTH];		/*Flawfinder: ignore*/
	agent_id.toString(buffer);

	gViewerWindow->mWindow->copyTextToClipboard(utf8str_to_wstring(buffer));
}


static void send_freeze(const LLUUID& avatar_id, bool freeze)
{
	U32 flags = 0x0;
	if (!freeze)
	{
		// unfreeze
		flags |= 0x1;
	}

	LLMessageSystem* msg = gMessageSystem;
	LLViewerObject* avatar = gObjectList.findObject(avatar_id);

	if (avatar)
	{
		msg->newMessage("FreezeUser");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID());
		msg->addUUID("SessionID", gAgent.getSessionID());
		msg->nextBlock("Data");
		msg->addUUID("TargetID", avatar_id );
		msg->addU32("Flags", flags );
		msg->sendReliable( avatar->getRegion()->getHost() );
	}
}

static void send_eject(const LLUUID& avatar_id, bool ban)
{
	LLMessageSystem* msg = gMessageSystem;
	LLViewerObject* avatar = gObjectList.findObject(avatar_id);

	if (avatar)
	{
		U32 flags = 0x0;
		if ( ban )
		{
			// eject and add to ban list
			flags |= 0x1;
		}

		msg->newMessage("EjectUser");
		msg->nextBlock("AgentData");
		msg->addUUID("AgentID", gAgent.getID() );
		msg->addUUID("SessionID", gAgent.getSessionID() );
		msg->nextBlock("Data");
		msg->addUUID("TargetID", avatar_id );
		msg->addU32("Flags", flags );
		msg->sendReliable( avatar->getRegion()->getHost() );
	}
}

static void send_estate_message(
	const char* request,
	const LLUUID &target)
{

	LLMessageSystem* msg = gMessageSystem;
	LLUUID invoice;

	// This seems to provide an ID so that the sim can say which request it's
	// replying to. I think this can be ignored for now.
	invoice.generate();

	llinfos << "Sending estate request '" << request << "'" << llendl;
	msg->newMessage("EstateOwnerMessage");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); //not used
	msg->nextBlock("MethodData");
	msg->addString("Method", request);
	msg->addUUID("Invoice", invoice);

	// Agent id
	msg->nextBlock("ParamList");
	msg->addString("Parameter", gAgent.getID().asString().c_str());

	// Target
	msg->nextBlock("ParamList");
	msg->addString("Parameter", target.asString().c_str());

	msg->sendReliable(gAgent.getRegion()->getHost());
}

static void send_estate_ban(const LLUUID& agent)
{
	LLUUID invoice;
	U32 flags = ESTATE_ACCESS_BANNED_AGENT_ADD;

	invoice.generate();

	LLMessageSystem* msg = gMessageSystem;
	msg->newMessage("EstateOwnerMessage");
	msg->nextBlockFast(_PREHASH_AgentData);
	msg->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
	msg->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
	msg->addUUIDFast(_PREHASH_TransactionID, LLUUID::null); //not used

	msg->nextBlock("MethodData");
	msg->addString("Method", "estateaccessdelta");
	msg->addUUID("Invoice", invoice);

	char buf[MAX_STRING];		/* Flawfinder: ignore*/
	gAgent.getID().toString(buf);
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buf);

	snprintf(buf, MAX_STRING, "%u", flags);			/* Flawfinder: ignore */
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buf);

	agent.toString(buf);
	msg->nextBlock("ParamList");
	msg->addString("Parameter", buf);

	gAgent.sendReliableMessage();
}

static void cmd_freeze(const LLUUID& avatar, const std::string &name)      { send_freeze(avatar, true); }
static void cmd_unfreeze(const LLUUID& avatar, const std::string &name)    { send_freeze(avatar, false); }
static void cmd_eject(const LLUUID& avatar, const std::string &name)       { send_eject(avatar, false); }
static void cmd_ban(const LLUUID& avatar, const std::string &name)         { send_eject(avatar, true); }
static void cmd_profile(const LLUUID& avatar, const std::string &name)     { LLFloaterAvatarInfo::showFromDirectory(avatar); }
static void cmd_mute(const LLUUID&avatar, const std::string &name)         { LLMuteList::getInstance()->add(LLMute(avatar, name, LLMute::AGENT)); }
static void cmd_unmute(const LLUUID&avatar, const std::string &name)       { LLMuteList::getInstance()->remove(LLMute(avatar, name, LLMute::AGENT)); }
static void cmd_estate_eject(const LLUUID &avatar, const std::string &name){ send_estate_message("teleporthomeuser", avatar); }
static void cmd_estate_ban(const LLUUID &avatar, const std::string &name)
{
	send_estate_message("teleporthomeuser", avatar); // Kick first, just to be sure
	send_estate_ban(avatar);
}

void LLFloaterAvatarList::doCommand(void (*func)(const LLUUID &avatar, const std::string &name))
{
	LLDynamicArray<LLUUID> ids = mAvatarList->getSelectedIDs();

	for(LLDynamicArray<LLUUID>::iterator itr = ids.begin(); itr != ids.end(); ++itr)
	{
		LLUUID avid = *itr;
		LLAvatarListEntry *ent = getAvatarEntry(avid);
		if ( ent != NULL )
		{
			llinfos << "Executing command on " << ent->getName() << llendl;
			func(avid, ent->getName());
		}
	}
}

std::string LLFloaterAvatarList::getSelectedNames(const std::string& separator)
{
	std::string ret = "";

	LLDynamicArray<LLUUID> ids = mAvatarList->getSelectedIDs();
	for(LLDynamicArray<LLUUID>::iterator itr = ids.begin(); itr != ids.end(); ++itr)
	{
		LLUUID avid = *itr;
		LLAvatarListEntry *ent = getAvatarEntry(avid);
		if ( ent != NULL )
		{
			if (!ret.empty()) ret += separator;
			ret += ent->getName();
		}
	}

	return ret;
}

//static
void LLFloaterAvatarList::callbackFreeze(S32 option, void *userdata) {
	LLFloaterAvatarList *avlist = (LLFloaterAvatarList*)userdata;

	if ( option == 0 )
	{
		avlist->doCommand(cmd_freeze);
	}
	else if ( option == 1 )
	{
		avlist->doCommand(cmd_unfreeze);
	}
}

//static
void LLFloaterAvatarList::callbackEject(S32 option, void *userdata) {
	LLFloaterAvatarList *avlist = (LLFloaterAvatarList*)userdata;

	if ( option == 0 )
	{
		avlist->doCommand(cmd_eject);
	}
	else if ( option == 1 )
	{
		avlist->doCommand(cmd_ban);
	}
}

//static
void LLFloaterAvatarList::callbackMute(S32 option, void *userdata) {
	LLFloaterAvatarList *avlist = (LLFloaterAvatarList*)userdata;

	if ( option == 0 )
	{
		avlist->doCommand(cmd_mute);
	}
	else if ( option == 1 )
	{
		avlist->doCommand(cmd_unmute);
	}
}

//static
void LLFloaterAvatarList::callbackEjectFromEstate(S32 option, void *userdata) {
	LLFloaterAvatarList *avlist = (LLFloaterAvatarList*)userdata;

	if ( option == 0 )
	{
		avlist->doCommand(cmd_estate_eject);
	}
	else if ( option == 1 )
	{
		avlist->doCommand(cmd_estate_ban);
	}
}

//static
void LLFloaterAvatarList::onClickFreeze(void *userdata)
{
	LLStringUtil::format_map_t args;
	args["[NAMES]"] = ((LLFloaterAvatarList*)userdata)->getSelectedNames();
	gViewerWindow->alertXml("AvatarListFreezeAvatars", args, callbackFreeze, userdata);
}

//static
void LLFloaterAvatarList::onClickEject(void *userdata)
{
	LLStringUtil::format_map_t args;
	args["[NAMES]"] = ((LLFloaterAvatarList*)userdata)->getSelectedNames();
	gViewerWindow->alertXml("AvatarListEjectAvatars", args, callbackEject, userdata);
}

//static
void LLFloaterAvatarList::onClickMute(void *userdata)
{
	LLStringUtil::format_map_t args;
	args["[NAMES]"] = ((LLFloaterAvatarList*)userdata)->getSelectedNames();
	gViewerWindow->alertXml("AvatarListMuteAvatars", args, callbackMute, userdata);
}

//static
void LLFloaterAvatarList::onClickEjectFromEstate(void *userdata)
{
	LLStringUtil::format_map_t args;
	args["[NAMES]"] = ((LLFloaterAvatarList*)userdata)->getSelectedNames();
	gViewerWindow->alertXml("AvatarListEjectAvatarsFromEstate", args, callbackEjectFromEstate, userdata);
}



//static
void LLFloaterAvatarList::onClickAR(void *userdata)
{
	LLFloaterAvatarList *avlist = (LLFloaterAvatarList*)userdata;
	LLDynamicArray<LLUUID> ids = avlist->mAvatarList->getSelectedIDs();

	for(LLDynamicArray<LLUUID>::iterator itr = ids.begin(); itr != ids.end(); ++itr)
	{
		LLUUID avid = *itr;
		llinfos << "Adding " << avid << " to AR queue" << llendl;
		avlist->mARQueue.push( avid );
	}
}

// static
void LLFloaterAvatarList::onClickProfile(void* userdata)
{
	LLFloaterAvatarList *avlist = (LLFloaterAvatarList*)userdata;
	avlist->doCommand(cmd_profile);
}

//static
void LLFloaterAvatarList::onClickTeleport(void* userdata)
{
	LLFloaterAvatarList *avlist = (LLFloaterAvatarList*)userdata;
 	LLScrollListItem *item =   avlist->mAvatarList->getFirstSelected();

	if ( item )
	{
		LLUUID agent_id = item->getUUID();
		LLAvatarListEntry *ent = avlist->getAvatarEntry(agent_id);

		if ( ent )
		{
			llinfos << "Trying to teleport to " << ent->getName() << " at " << ent->getPosition() << llendl;
			gAgent.teleportViaLocation( ent->getPosition() );
		}
	}
}


void LLFloaterAvatarList::renderDebugBeacons()
{
	LLFastTimer t(LLFastTimer::FTM_TEMP1);
	std::map<LLUUID, LLAvatarListEntry>::iterator iter;
	for(iter = mAvatars.begin(); iter != mAvatars.end(); ++iter)
	{
		LLAvatarListEntry entry = iter->second;
		if ( entry.isDead() )
		{ // remove HUD Object if it exists
			if ( mHudObjectMap.count(entry.getID()) )
			{
				mHudObjectMap[entry.getID()]->markDead();
				mHudObjectMap.erase(entry.getID());
			}
			continue;
		}

		std::string name = entry.getName();
		LLVector3d avpos = entry.getPosition();
		LLVector3d mypos = gAgent.getPositionGlobal();
		LLVector3d delta = avpos - mypos;
		F32 distance = (F32)delta.magVec();

		std::string info = llformat( "%s %.02fm", name.c_str(), distance );
		LLVector3 agentpos = gAgent.getPosAgentFromGlobal( avpos );

		// roll our own HUD text since the debug beacons are rendered too late (after HUD update)
		if (mHudObjectMap.count(entry.getID()) == 0)
		{
			mHudObjectMap[entry.getID()] = LLHUDObject::addHUDObject(LLHUDObject::LL_HUD_TEXT);
		}
		LLHUDText* hud_textp = (LLHUDText *)mHudObjectMap[entry.getID()].get(); // :-/

		hud_textp->setZCompare(FALSE);
		hud_textp->setString(utf8str_to_wstring(info));
		hud_textp->setColor(LLColor4::white);
		hud_textp->setPositionAgent(agentpos);
		hud_textp->setUseBubble(TRUE);
		hud_textp->setMass(10.f);
		hud_textp->setDoFade(FALSE);
		//std::string posinfo = llformat("%f %f %f", agentpos[0], agentpos[1], agentpos[2]);
		//hud_textp->setLabel(posinfo);

		gObjectList.addDebugBeacon( agentpos, "", LLColor4(0.0f, 0.0f, 1.f, 0.5f), LLColor4::white, gSavedSettings.getS32("DebugBeaconLineWidth") );
	}
}

bool LLFloaterAvatarList::sRenderAvatarBeacons = true;