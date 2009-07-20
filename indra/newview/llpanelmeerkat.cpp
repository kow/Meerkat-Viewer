/** 
 * @file LLPanelMeerkat.cpp
 * @brief General preferences panel in preferences floater
 *
 * $LicenseInfo:firstyear=2001&license=viewergpl$
 * 
 * Copyright (c) 2001-2009, Linden Research, Inc.
 * 
 * Second Life Viewer Source Code
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
 * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlifegrid.net/programs/open_source/licensing/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 * $/LicenseInfo$
 */

#include "llviewerprecompiledheaders.h"

#include "llpanelmeerkat.h"
//#include "lggBeamMapFloater.h"
// linden library includes
#include "llradiogroup.h"
#include "llbutton.h"
#include "lluictrlfactory.h"
#include "llcombobox.h"
#include "llslider.h"
#include "lltexturectrl.h"

//#include "lggBeamMaps.h"

// project includes
#include "llviewercontrol.h"
#include "llviewerwindow.h"
#include "llsdserialize.h"


#include "lltabcontainer.h"

#include "llinventorymodel.h"
#include "llfilepicker.h"
#include "llstartup.h"

#include "lltexteditor.h"

#include "llagent.h"

////////begin drop utility/////////////
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
// Class JCInvDropTarget
//
// This handy class is a simple way to drop something on another
// view. It handles drop events, always setting itself to the size of
// its parent.
//
// altered to support a callback so i can slap it in things and it just return the item to a func of my choice
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

class JCInvDropTarget : public LLView
{
public:
	JCInvDropTarget(const std::string& name, const LLRect& rect, void (*callback)(LLViewerInventoryItem*));
	~JCInvDropTarget();

	void doDrop(EDragAndDropType cargo_type, void* cargo_data);

	//
	// LLView functionality
	virtual BOOL handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
								   EDragAndDropType cargo_type,
								   void* cargo_data,
								   EAcceptance* accept,
								   std::string& tooltip_msg);
protected:
	void	(*mDownCallback)(LLViewerInventoryItem*);
};


JCInvDropTarget::JCInvDropTarget(const std::string& name, const LLRect& rect,
						  void (*callback)(LLViewerInventoryItem*)) :
	LLView(name, rect, NOT_MOUSE_OPAQUE, FOLLOWS_ALL),
	mDownCallback(callback)
{
}

JCInvDropTarget::~JCInvDropTarget()
{
}

void JCInvDropTarget::doDrop(EDragAndDropType cargo_type, void* cargo_data)
{
	llinfos << "JCInvDropTarget::doDrop()" << llendl;
}

BOOL JCInvDropTarget::handleDragAndDrop(S32 x, S32 y, MASK mask, BOOL drop,
									 EDragAndDropType cargo_type,
									 void* cargo_data,
									 EAcceptance* accept,
									 std::string& tooltip_msg)
{
	BOOL handled = FALSE;
	if(getParent())
	{
		handled = TRUE;
		// check the type
		//switch(cargo_type)
		//{
		//case DAD_ANIMATION:
		//{
			LLViewerInventoryItem* inv_item = (LLViewerInventoryItem*)cargo_data;
			if(gInventory.getItem(inv_item->getUUID()))
			{
				*accept = ACCEPT_YES_COPY_SINGLE;
				if(drop)
				{
					//printchat("accepted");
					mDownCallback(inv_item);
				}
			}
			else
			{
				*accept = ACCEPT_NO;
			}
		//	break;
		//}
		//default:
		//	*accept = ACCEPT_NO;
		//	break;
		//}
	}
	return handled;
}
////////end drop utility///////////////

LLPanelMeerkat* LLPanelMeerkat::sInstance;

JCInvDropTarget * LLPanelMeerkat::mObjectDropTarget;

LLPanelMeerkat::LLPanelMeerkat()
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_preferences_Meerkat.xml");
	if(sInstance)delete sInstance;
	sInstance = this;
}

LLPanelMeerkat::~LLPanelMeerkat()
{
	sInstance = NULL;
	delete mObjectDropTarget;
	mObjectDropTarget = NULL;
}

BOOL LLPanelMeerkat::postBuild()
{
	refresh();
	getChild<LLComboBox>("material")->setSimple(gSavedSettings.getString("MeerkatBuildPrefs_Material"));
	getChild<LLComboBox>("combobox shininess")->setSimple(gSavedSettings.getString("MeerkatBuildPrefs_Shiny"));
	
	getChild<LLSlider>("MeerkatBeamShapeScale")->setCommitCallback(beamUpdateCall);
	getChild<LLSlider>("MeerkatMaxBeamsPerSecond")->setCommitCallback(beamUpdateCall);

	getChild<LLComboBox>("material")->setCommitCallback(onComboBoxCommit);
	getChild<LLComboBox>("combobox shininess")->setCommitCallback(onComboBoxCommit);
	getChild<LLComboBox>("MeerkatBeamShape_combo")->setCommitCallback(onComboBoxCommit);
	getChild<LLTextureCtrl>("texture control")->setDefaultImageAssetID(LLUUID("89556747-24cb-43ed-920b-47caed15465f"));
	getChild<LLTextureCtrl>("texture control")->setCommitCallback(onTexturePickerCommit);

		
	//childSetCommitCallback("material",onComboBoxCommit);
	//childSetCommitCallback("combobox shininess",onComboBoxCommit);
	getChild<LLButton>("custom_beam_btn")->setClickedCallback(onCustomBeam, this);
	getChild<LLButton>("refresh_beams")->setClickedCallback(onRefresh,this);
	getChild<LLButton>("delete_beam")->setClickedCallback(onBeamDelete,this);
	getChild<LLButton>("revert_production_voice_btn")->setClickedCallback(onClickVoiceRevertProd, this);
	getChild<LLButton>("revert_debug_voice_btn")->setClickedCallback(onClickVoiceRevertDebug, this);
	

	childSetCommitCallback("production_voice_field", onCommitApplyControl);//onCommitVoiceProductionServerName);
	childSetCommitCallback("debug_voice_field", onCommitApplyControl);//onCommitVoiceDebugServerName);

	childSetCommitCallback("MeerkatCmdLinePos", onCommitApplyControl);
	childSetCommitCallback("MeerkatCmdLineGround", onCommitApplyControl);
	childSetCommitCallback("MeerkatCmdLineHeight", onCommitApplyControl);
	childSetCommitCallback("MeerkatCmdLineTeleportHome", onCommitApplyControl);
	childSetCommitCallback("MeerkatCmdLineRezPlatform", onCommitApplyControl);
	childSetCommitCallback("MeerkatCmdLineMapTo", onCommitApplyControl);	
	childSetCommitCallback("MeerkatCmdLineCalc", onCommitApplyControl);

	childSetCommitCallback("MeerkatCmdLineDrawDistance", onCommitApplyControl);
	childSetCommitCallback("MeerkatCmdTeleportToCam", onCommitApplyControl);
	childSetCommitCallback("MeerkatCmdLineKeyToName", onCommitApplyControl);
	childSetCommitCallback("MeerkatCmdLineOfferTp", onCommitApplyControl);

	childSetCommitCallback("X Modifier", onCommitSendAppearance);
	childSetCommitCallback("Y Modifier", onCommitSendAppearance);
	childSetCommitCallback("Z Modifier", onCommitSendAppearance);

	LLView *target_view = getChild<LLView>("im_give_drop_target_rect");
	if(target_view)
	{
		if (mObjectDropTarget)//shouldn't happen
		{
			delete mObjectDropTarget;
		}
		mObjectDropTarget = new JCInvDropTarget("drop target", target_view->getRect(), IMAutoResponseItemDrop);//, mAvatarID);
		addChild(mObjectDropTarget);
	}

	if(LLStartUp::getStartupState() == STATE_STARTED)
	{
		LLUUID itemid = (LLUUID)gSavedPerAccountSettings.getString("MeerkatInstantMessageResponseItemData");
		LLViewerInventoryItem* item = gInventory.getItem(itemid);
		if(item)
		{
			childSetValue("im_give_disp_rect_txt","Currently set to: "+item->getName());
		}else if(itemid.isNull())
		{
			childSetValue("im_give_disp_rect_txt","Currently not set");
		}else
		{
			childSetValue("im_give_disp_rect_txt","Currently set to a item not on this account");
		}
	}else
	{
		childSetValue("im_give_disp_rect_txt","Not logged in");
	}

	LLWString auto_response = utf8str_to_wstring( gSavedPerAccountSettings.getString("MeerkatInstantMessageResponse") );
	LLWStringUtil::replaceChar(auto_response, '^', '\n');
	LLWStringUtil::replaceChar(auto_response, '%', ' ');
	childSetText("im_response", wstring_to_utf8str(auto_response));
	childSetValue("MeerkatInstantMessageResponseFriends", gSavedPerAccountSettings.getBOOL("MeerkatInstantMessageResponseFriends"));
	childSetValue("MeerkatInstantMessageResponseMuted", gSavedPerAccountSettings.getBOOL("MeerkatInstantMessageResponseMuted"));
	childSetValue("MeerkatInstantMessageResponseAnyone", gSavedPerAccountSettings.getBOOL("MeerkatInstantMessageResponseAnyone"));
	childSetValue("MeerkatInstantMessageShowResponded", gSavedPerAccountSettings.getBOOL("MeerkatInstantMessageShowResponded"));
	childSetValue("MeerkatInstantMessageShowOnTyping", gSavedPerAccountSettings.getBOOL("MeerkatInstantMessageShowOnTyping"));
	childSetValue("MeerkatInstantMessageResponseRepeat", gSavedPerAccountSettings.getBOOL("MeerkatInstantMessageResponseRepeat" ));
	childSetValue("MeerkatInstantMessageResponseItem", gSavedPerAccountSettings.getBOOL("MeerkatInstantMessageResponseItem"));
	childSetValue("MeerkatInstantMessageAnnounceIncoming", gSavedPerAccountSettings.getBOOL("MeerkatInstantMessageAnnounceIncoming"));
	childSetValue("MeerkatInstantMessageAnnounceStealFocus", gSavedPerAccountSettings.getBOOL("MeerkatInstantMessageAnnounceStealFocus"));
	childSetValue("multiple-viewers-toggle", gSavedSettings.getBOOL("AllowMultipleViewers"));

	refresh();
	return TRUE;
}

void LLPanelMeerkat::refresh()
{
	/* KOW uncomment this stuff when implementing LGG's beam code

	LLComboBox* comboBox = getChild<LLComboBox>("MeerkatBeamShape_combo");

	if(comboBox != NULL) 
	{
		comboBox->removeall();
		comboBox->add("===OFF===");
		std::vector<std::string> names = gLggBeamMaps.getFileNames();
		for(int i=0; i<(int)names.size(); i++) 
		{
			comboBox->add(names[i]);
		}
		comboBox->setSimple(gSavedSettings.getString("MeerkatBeamShape"));
	}
	*/
	//mSkin = gSavedSettings.getString("SkinCurrent");
	//getChild<LLRadioGroup>("skin_selection")->setValue(mSkin);
}

void LLPanelMeerkat::IMAutoResponseItemDrop(LLViewerInventoryItem* item)
{
	gSavedPerAccountSettings.setString("MeerkatInstantMessageResponseItemData", item->getUUID().asString());
	sInstance->childSetValue("im_give_disp_rect_txt","Currently set to: "+item->getName());
}

void LLPanelMeerkat::apply()
{
	LLTextEditor* im = sInstance->getChild<LLTextEditor>("im_response");
	LLWString im_response;
	if (im) im_response = im->getWText(); 
	LLWStringUtil::replaceTabsWithSpaces(im_response, 4);
	LLWStringUtil::replaceChar(im_response, '\n', '^');
	LLWStringUtil::replaceChar(im_response, ' ', '%');
	gSavedPerAccountSettings.setString("MeerkatInstantMessageResponse", std::string(wstring_to_utf8str(im_response)));

	//gSavedPerAccountSettings.setString(
	gSavedPerAccountSettings.setBOOL("MeerkatInstantMessageResponseFriends", childGetValue("MeerkatInstantMessageResponseFriends").asBoolean());
	gSavedPerAccountSettings.setBOOL("MeerkatInstantMessageResponseMuted", childGetValue("MeerkatInstantMessageResponseMuted").asBoolean());
	gSavedPerAccountSettings.setBOOL("MeerkatInstantMessageResponseAnyone", childGetValue("MeerkatInstantMessageResponseAnyone").asBoolean());
	gSavedPerAccountSettings.setBOOL("MeerkatInstantMessageShowResponded", childGetValue("MeerkatInstantMessageShowResponded").asBoolean());
	gSavedPerAccountSettings.setBOOL("MeerkatInstantMessageShowOnTyping", childGetValue("MeerkatInstantMessageShowOnTyping").asBoolean());
	gSavedPerAccountSettings.setBOOL("MeerkatInstantMessageResponseRepeat", childGetValue("MeerkatInstantMessageResponseRepeat").asBoolean());
	gSavedPerAccountSettings.setBOOL("MeerkatInstantMessageResponseItem", childGetValue("MeerkatInstantMessageResponseItem").asBoolean());
	gSavedPerAccountSettings.setBOOL("MeerkatInstantMessageAnnounceIncoming", childGetValue("MeerkatInstantMessageAnnounceIncoming").asBoolean());
	gSavedPerAccountSettings.setBOOL("MeerkatInstantMessageAnnounceStealFocus", childGetValue("MeerkatInstantMessageAnnounceStealFocus").asBoolean());
	gSavedSettings.setBOOL("AllowMultipleViewers", childGetValue("multiple-viewers-toggle").asBoolean());
	
//kow uncomment when fixing lgg beam	gLggBeamMaps.forceUpdate();
}

void LLPanelMeerkat::cancel()
{
}
void LLPanelMeerkat::onClickVoiceRevertProd(void* data)
{
	LLPanelMeerkat* self = (LLPanelMeerkat*)data;
	gSavedSettings.setString("vivoxProductionServerName", "bhr.vivox.com");
	self->getChild<LLLineEditor>("production_voice_field")->setValue("bhr.vivox.com");
}
void LLPanelMeerkat::onCustomBeam(void* data)
{
	//LLPanelMeerkat* self =(LLPanelMeerkat*)data;
//kow uncomment when fixing lgg beam	LggBeamMap::show(true);

}
void LLPanelMeerkat::beamUpdateCall(LLUICtrl* crtl, void* userdata)
{
//kow uncomment when fixing lgg beam	gLggBeamMaps.forceUpdate();
}
void LLPanelMeerkat::onComboBoxCommit(LLUICtrl* ctrl, void* userdata)
{
	LLComboBox* box = (LLComboBox*)ctrl;
	if(box)
	{
		gSavedSettings.setString(box->getControlName(),box->getValue().asString());
	}
	
}
void LLPanelMeerkat::onTexturePickerCommit(LLUICtrl* ctrl, void* userdata)
{
	LLTextureCtrl*	image_ctrl = (LLTextureCtrl*)ctrl;
	if(image_ctrl)
	{
		gSavedSettings.setString("MeerkatBuildPrefs_Texture", image_ctrl->getImageAssetID().asString());
	}
}

void LLPanelMeerkat::onRefresh(void* data)
{
	LLPanelMeerkat* self = (LLPanelMeerkat*)data;
	self->refresh();
	
	
}
void LLPanelMeerkat::onClickVoiceRevertDebug(void* data)
{
	LLPanelMeerkat* self = (LLPanelMeerkat*)data;
	gSavedSettings.setString("vivoxDebugServerName", "bhd.vivox.com");
	self->getChild<LLLineEditor>("debug_voice_field")->setValue("bhd.vivox.com");

 
 
}
void LLPanelMeerkat::onBeamDelete(void* data)
{
	LLPanelMeerkat* self = (LLPanelMeerkat*)data;
	
	LLComboBox* comboBox = self->getChild<LLComboBox>("MeerkatBeamShape_combo");

	if(comboBox != NULL) 
	{
		std::string filename = gDirUtilp->getAppRODataDir() 
						+gDirUtilp->getDirDelimiter()
						+"beams"
						+gDirUtilp->getDirDelimiter()
						+comboBox->getValue().asString()+".xml";
		if(gDirUtilp->fileExists(filename))
		{
			LLFile::remove(filename);
			gSavedSettings.setString("MeerkatBeamShape","===OFF===");
		}
	}
	self->refresh();
}

//workaround for lineeditor dumbness in regards to control_name
void LLPanelMeerkat::onCommitApplyControl(LLUICtrl* caller, void* user_data)
{
	LLLineEditor* line = (LLLineEditor*)caller;
	if(line)
	{
		LLControlVariable *var = line->findControl(line->getControlName());
		if(var)var->setValue(line->getValue());
	}
}

void LLPanelMeerkat::onCommitSendAppearance(LLUICtrl* ctrl, void* userdata)
{
	gAgent.sendAgentSetAppearance();
	//llinfos << llformat("%d,%d,%d",gSavedSettings.getF32("MeerkatAvatarXModifier"),gSavedSettings.getF32("MeerkatAvatarYModifier"),gSavedSettings.getF32("MeerkatAvatarZModifier")) << llendl;
}


/*
//static 
void LLPanelMeerkat::onClickSilver(void* data)
{
	LLPanelMeerkat* self = (LLPanelMeerkat*)data;
	gSavedSettings.setString("SkinCurrent", "silver");
	self->getChild<LLRadioGroup>("skin_selection")->setValue("silver");
}*/
