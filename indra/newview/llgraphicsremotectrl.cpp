/** 
 * @file LLGraphicsRemotectrl.cpp
 * @author Patrick Sapinski
 * @date Thursday, August 20, 2009
 * @brief A remote control for graphics settings
 *
 */

#include "llviewerprecompiledheaders.h"

#include "llgraphicsremotectrl.h"

#include "llcombobox.h"
#include "lliconctrl.h"
#include "llmimetypes.h"
#include "lloverlaybar.h"
#include "lluictrlfactory.h"
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

	//mFactoryMap["Graphics Panel"]	= LLCallbackMap(createGraphicsPanel, NULL);
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
	
	childSetAction("ShowGraphicsPopup", onClickExpandBtn, this);	

	// add the combo boxes
	LLComboBox* comboBox = getChild<LLComboBox>("WLPresetsCombo");

	if(comboBox != NULL) {
		
		std::map<std::string, LLWLParamSet>::iterator mIt = 
			LLWLParamManager::instance()->mParamList.begin();
		for(; mIt != LLWLParamManager::instance()->mParamList.end(); mIt++) 
		{
			comboBox->add(mIt->first);
		}

		// entry for when we're in estate time
		comboBox->add(LLStringUtil::null);

		// set defaults on combo boxes
		comboBox->selectByValue(LLSD("Default"));
	}
	comboBox->setCommitCallback(onChangePresetName);

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
/*
void* LLGraphicsRemoteCtrl::createGraphicsPanel(void* data)
{
	LLPanelAudioVolume* panel = new LLPanelAudioVolume();
	return panel;
}
*/

void LLGraphicsRemoteCtrl::onChangePresetName(LLUICtrl* ctrl, void * userData)
{
	LLWLParamManager::instance()->mAnimator.mIsRunning = false;
	LLWLParamManager::instance()->mAnimator.mUseLindenTime = false;

	LLComboBox * combo_box = static_cast<LLComboBox*>(ctrl);
	
	if(combo_box->getSimple() == "")
	{
		return;
	}
	
	LLWLParamManager::instance()->loadPreset(
		combo_box->getSelectedValue().asString());
	//sWindLight->syncMenu();
}

// Virtual
void LLGraphicsRemoteCtrl::setToolTip(const std::string& msg)
{
	//std::string mime_type = LLMIMETypes::translate(LLViewerMedia::getMimeType());
	//std::string tool_tip = LLMIMETypes::findToolTip(LLViewerMedia::getMimeType());
	//std::string play_tip = LLMIMETypes::findPlayTip(LLViewerMedia::getMimeType());
	// childSetToolTip("media_stop", mControls->getString("stop_label") + "\n" + tool_tip);
	//childSetToolTip("media_icon", tool_tip);
	//childSetToolTip("media_play", play_tip);
}

void LLGraphicsRemoteCtrl::enableGraphicsButtons()
{
	LLColor4 music_icon_color = LLUI::sColorsGroup->getColor( "IconDisabledColor" );
	LLColor4 media_icon_color = LLUI::sColorsGroup->getColor( "IconDisabledColor" );
	std::string media_type = "none/none";

	// Put this in xui file
	//std::string media_url = mControls->getString("default_tooltip_label");
	//LLParcel* parcel = LLViewerParcelMgr::getInstance()->getAgentParcel();
/*
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
*/
	//childSetColor("music_icon", music_icon_color);
	//childSetColor("media_icon", media_icon_color);

	//setToolTip(media_url);
}


