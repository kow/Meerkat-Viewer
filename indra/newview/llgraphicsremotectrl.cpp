/** 
 * @file LLGraphicsRemotectrl.cpp
 * @brief A remote control for media (video and music)
 *
 * $LicenseInfo:firstyear=2005&license=viewergpl$
 * 
 * Copyright (c) 2005-2008, Linden Research, Inc.
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

#include "LLGraphicsRemotectrl.h"

#include "audioengine.h"
#include "lliconctrl.h"
#include "llmimetypes.h"
#include "lloverlaybar.h"
#include "llviewermedia.h"
#include "llviewerparcelmedia.h"
#include "llviewerparcelmgr.h"
#include "lluictrlfactory.h"
#include "llpanelaudiovolume.h"
#include "llparcel.h"
#include "llviewercontrol.h"
#include "llbutton.h"
#include "llsliderctrl.h"
#include "llwlparammanager.h"

////////////////////////////////////////////////////////////////////////////////
//
//

static LLRegisterWidget<LLGraphicsRemoteCtrl> r("graphics_remote");

LLGraphicsRemoteCtrl::LLGraphicsRemoteCtrl()
{
	setIsChrome(TRUE);
	setFocusRoot(TRUE);

	mFactoryMap["Graphics Panel"]	= LLCallbackMap(createGraphicsPanel, NULL);
	build();
}

void LLGraphicsRemoteCtrl::build()
{
	//HACK: only works because we don't have any implicit children (i.e. titlebars, close button, etc)
	deleteAllChildren();
	if (gSavedSettings.getBOOL("ShowGraphicsSettingsPopup"))
	{
		LLUICtrlFactory::getInstance()->buildPanel(this, "panel_graphics_remote_expanded.xml", &getFactoryMap());
	}
	else
	{
		LLUICtrlFactory::getInstance()->buildPanel(this, "panel_graphics_remote.xml", &getFactoryMap());
	}
}

BOOL LLGraphicsRemoteCtrl::postBuild()
{
	mControls = getChild<LLPanel>("graphics_controls");
	llassert_always(mControls);
	
	// our three sliders
	childSetCommitCallback("EnvTimeSlider", onChangeDayTime, NULL);

	childSetAction("media_play",LLOverlayBar::toggleMediaPlay,this);
	childSetAction("music_play",LLOverlayBar::toggleMusicPlay,this);
	childSetAction("media_stop",LLOverlayBar::mediaStop,this);
	childSetAction("music_stop",LLOverlayBar::toggleMusicPlay,this);
	childSetAction("media_pause",LLOverlayBar::toggleMediaPlay,this);
	childSetAction("music_pause",LLOverlayBar::toggleMusicPlay,this);

	childSetAction("ShowGraphicsPopup", onClickExpandBtn, this);	
	return TRUE;
}

void LLGraphicsRemoteCtrl::draw()
{
	enableGraphicsButtons();
	
	LLButton* expand_button = getChild<LLButton>("ShowGraphicsPopup");
	if (expand_button)
	{
		if (expand_button->getToggleState())
		{
			expand_button->setImageOverlay("arrow_down.tga");
		}
		else
		{
			expand_button->setImageOverlay("arrow_up.tga");
		}
	}

	LLPanel::draw();
}

LLGraphicsRemoteCtrl::~LLGraphicsRemoteCtrl ()
{
}

//static 
void LLGraphicsRemoteCtrl::onClickExpandBtn(void* user_data)
{
	LLGraphicsRemoteCtrl* remotep = (LLGraphicsRemoteCtrl*)user_data;

	remotep->build();
	gOverlayBar->layoutButtons();

}

//static
void* LLGraphicsRemoteCtrl::createGraphicsPanel(void* data)
{
	LLPanelAudioVolume* panel = new LLPanelAudioVolume();
	return panel;
}

void LLGraphicsRemoteCtrl::onChangeDayTime(LLUICtrl* ctrl, void* userData)
{

	LLSliderCtrl* sldr = (LLSliderCtrl*) ctrl;
	//LLGraphicsRemoteCtrl* self = (LLGraphicsRemoteCtrl*) userData;
	//LLSliderCtrl* sldr = self->getChild<LLSliderCtrl>("EnvTimeSlider");

	if (sldr) {
		// deactivate animator
		LLWLParamManager::instance()->mAnimator.mIsRunning = false;
		LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;

		F32 val = sldr->getValueF32() + 0.25f;
		if(val > 1.0) 
		{
			val--;
		}

		LLWLParamManager::instance()->mAnimator.setDayTime((F64)val);
		LLWLParamManager::instance()->mAnimator.update(
			LLWLParamManager::instance()->mCurParams);
	}
}


// Virtual
void LLGraphicsRemoteCtrl::setToolTip(const std::string& msg)
{
	std::string mime_type = LLMIMETypes::translate(LLViewerMedia::getMimeType());
	std::string tool_tip = LLMIMETypes::findToolTip(LLViewerMedia::getMimeType());
	std::string play_tip = LLMIMETypes::findPlayTip(LLViewerMedia::getMimeType());
	// childSetToolTip("media_stop", mControls->getString("stop_label") + "\n" + tool_tip);
	childSetToolTip("media_icon", tool_tip);
	childSetToolTip("media_play", play_tip);
}

void LLGraphicsRemoteCtrl::enableGraphicsButtons()
{
	LLColor4 music_icon_color = LLUI::sColorsGroup->getColor( "IconDisabledColor" );
	LLColor4 media_icon_color = LLUI::sColorsGroup->getColor( "IconDisabledColor" );
	std::string media_type = "none/none";

	// Put this in xui file
	std::string media_url = mControls->getString("default_tooltip_label");
	LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();

	if (gSavedSettings.getBOOL("AudioStreamingVideo"))
	{
		if ( parcel && parcel->getMediaURL()[0])
		{
			// Set the tooltip
			// Put this text into xui file
			media_url = parcel->getObscureMedia() ? mControls->getString("media_hidden_label") : parcel->getMediaURL();
			media_type = parcel->getMediaType();

			media_icon_color = LLUI::sColorsGroup->getColor( "IconEnabledColor" );
		}
	}

	childSetColor("music_icon", music_icon_color);
	childSetColor("media_icon", media_icon_color);

	setToolTip(media_url);
}


