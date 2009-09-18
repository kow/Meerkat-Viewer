/** 
 * @file llpaneldatabase.cpp
 * @brief Database preferences panel
 * @author Dale Glass
 *
 * Copyright (c) 2003-2007, Linden Research, Inc.
 * 
 * The source code in this file ("Source Code") is provided by Linden Lab
 * to you under the terms of the GNU General Public License, version 2.0
 * ("GPL"), unless you have obtained a separate licensing agreement
 * ("Other License"), formally executed by you and Linden Lab.  Terms of
+ * the GPL can be found in doc/GPL-license.txt in this distribution, or
 * online at http://secondlife.com/developers/opensource/gplv2
 * 
 * There are special exceptions to the terms and conditions of the GPL as
 * it is applied to this Source Code. View the full text of the exception
 * in the file doc/FLOSS-exception.txt in this software distribution, or
 * online at http://secondlife.com/developers/opensource/flossexception
 * 
 * By copying, modifying or distributing this software, you acknowledge
 * that you have read and understood your obligations described above,
 * and agree to abide by those obligations.
 * 
 * ALL LINDEN LAB SOURCE CODE IS PROVIDED "AS IS." LINDEN LAB MAKES NO
 * WARRANTIES, EXPRESS, IMPLIED OR OTHERWISE, REGARDING ITS ACCURACY,
 * COMPLETENESS OR PERFORMANCE.
 */

#include "llviewerprecompiledheaders.h"

#include "llpaneldatabase.h"

#include "llscrolllistctrl.h"
#include "llviewerwindow.h"
#include "llviewercontrol.h"
#include "lluictrlfactory.h"
#include "llfloateravatarpicker.h"
#include "llagentdata.h"
#include "llnotify.h"

//-----------------------------------------------------------------------------
LLPanelDatabase::LLPanelDatabase() :
	LLPanel("Messages Panel")
{
	LLUICtrlFactory::getInstance()->buildPanel(this, "panel_preferences_database.xml");
	childSetAction("change_avatar_btn", onClickChangeAvatar, this);
};

//-----------------------------------------------------------------------------
// postBuild()
//-----------------------------------------------------------------------------

void LLPanelDatabase::refresh() 
{
	llinfos << "Loading settings" << llendl;
	//mAvatarName      = gSavedPerAccountSettings.getString("DBAvatarName");
	//mAvatarKey.set(gSavedPerAccountSettings.getString("DBAvatarKey"));
	//mURL             = gSavedSettings.getString("DBURL");;
	//mUsername        = gSavedSettings.getString("DBURLUsername");;
	//mPassword        = gSavedSettings.getString("DBURLPassword");;
	//mSendPattern     = gSavedSettings.getString("DBSendPattern");;
	//mPositivePattern = gSavedSettings.getString("DBPositivePattern");;
	//mNegativePattern = gSavedSettings.getString("DBNegativePattern");;
	//mDeniedPattern   = gSavedSettings.getString("DBDeniedPattern");;

}

BOOL LLPanelDatabase::postBuild()
{
	refresh();

	llinfos << "Setting settings in window" << llendl;
	childSetText("db_avatar"       ,mAvatarName );
	childSetText("db_url"          ,mURL );
	childSetText("db_url_username" ,mUsername );
	childSetText("db_url_password" ,mPassword );
	childSetText("send_pattern"    ,mSendPattern );
	childSetText("positive_pattern",mPositivePattern );
	childSetText("negative_pattern",mNegativePattern );
	childSetText("denied_pattern"  ,mDeniedPattern );

	return TRUE;
}


void LLPanelDatabase::draw()
{
	LLPanel::draw();
}

void LLPanelDatabase::apply()
{
	//llinfos << "Saving settings" << llendl;

	//gSavedPerAccountSettings.setString("DBAvatarName", childGetText("db_avatar").c_str());
	//gSavedPerAccountSettings.setString("DBAvatarKey", mAvatarKey.asString());
	//gSavedSettings.setString("DBURL", childGetText("db_url").c_str());
	//gSavedSettings.setString("DBURLUsername", childGetText("db_url_username").c_str());
	//gSavedSettings.setString("DBURLPassword", childGetText("db_url_password").c_str());

	//gSavedSettings.setString("DBSendPattern", childGetText("send_pattern").c_str());
	//gSavedSettings.setString("DBPositivePattern", childGetText("positive_pattern").c_str());
	//gSavedSettings.setString("DBNegativePattern", childGetText("negative_pattern").c_str());
	//gSavedSettings.setString("DBDeniedPattern", childGetText("denied_pattern").c_str());	


}

void LLPanelDatabase::cancel()
{
	
}

//static
void LLPanelDatabase::onClickChangeAvatar(void *userdata)
{
	LLFloaterAvatarPicker::show(onPickAvatar, userdata, FALSE, TRUE);
}

//static
void LLPanelDatabase::onPickAvatar(const std::vector<std::string>& names,
                                   const std::vector<LLUUID>& ids,
                                   void* user_data)
{
	if (names.empty()) return;
	if (ids.empty()) return;

	LLPanelDatabase *self = (LLPanelDatabase*)user_data;

#ifndef LL_DEBUG
	// TODO: LL_DEBUG isn't the right one, what is it?
	//
	// Using yourself as the database avatar should work, and be useful
	// for debugging, but it's not something a normal user should be able
	// to do.
	
//	if(ids[0] == gAgentID)
//	{
//		LLNotifyBox::showXml("AddSelfDatabase");
//		return;
//	}
#endif

	self->childSetText("db_avatar", names[0]);
	self->mAvatarName = names[0];
	self->mAvatarKey = ids[0];
}

