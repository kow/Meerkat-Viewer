/*
 *  floaterlogin.cpp
 *  SecondLife
 *
 *  Created by RMS on 7/15/08.
 *
 */

#include "llviewerprecompiledheaders.h"

#include <boost/algorithm/string.hpp>
#include "llviewercontrol.h"
#include "llviewerbuild.h"
#include "llcombobox.h"
#include "llmd5.h"
#include "llurlsimstring.h"
#include "lluictrlfactory.h"
#include "controllerlogin.h"
#include "authentication_model.h"
#include "floaterlogin.h"

LoginFloater* LoginFloater::sInstance = NULL;
LoginController* LoginFloater::sController = NULL;
AuthenticationModel* LoginFloater::sModel = NULL;
bool LoginFloater::sIsInitialLogin;
std::string LoginFloater::sGrid;

LoginFloater::LoginFloater(void (*callback)(S32 option, void* user_data),
						   void *cb_data)
:	LLFloater("floater_login"), mCallback(callback), mCallbackData(cb_data)
{
	LLUICtrlFactory::getInstance()->buildFloater(this, "floater_login.xml");
	
	// configure the floater interface for non-initial login
	setCanMinimize(!sIsInitialLogin);
	setCanClose(!sIsInitialLogin);
	setCanDrag(!sIsInitialLogin);
	childSetVisible("server_combo", sIsInitialLogin);
	
	if(!sIsInitialLogin)
	{
		LLButton* quit_btn = getChild<LLButton>("quit_btn");
		quit_btn->setLabel(std::string("Cancel"));
		setTitle(sGrid + std::string(" authentication"));
	}
	
	center();
	LLLineEditor* edit = getChild<LLLineEditor>("password_edit");
	if (edit) edit->setDrawAsterixes(TRUE);
	LLComboBox* combo = getChild<LLComboBox>("start_location_combo");
	combo->setAllowTextEntry(TRUE, 128, FALSE);
	
	BOOL login_last = gSavedSettings.getBOOL("LoginLastLocation");
	std::string sim_string = LLURLSimString::sInstance.mSimString;
	if (!sim_string.empty())
	{
		// Replace "<Type region name>" with this region name
		combo->remove(2);
		combo->add( sim_string );
		combo->setTextEntry(sim_string);
		combo->setCurrentByIndex( 2 );
	}
	else if (login_last)
	{
		combo->setCurrentByIndex( 1 );
	}
	else
	{
		combo->setCurrentByIndex( 0 );
	}
	
	LLTextBox* version_text = getChild<LLTextBox>("version_text");
	std::string version = llformat("%d.%d.%d (%d)",
								   LL_VERSION_MAJOR,
								   LL_VERSION_MINOR,
								   LL_VERSION_PATCH,
								   LL_VIEWER_BUILD );
	version_text->setText(version);
	
	LLTextBox* channel_text = getChild<LLTextBox>("channel_text");
	channel_text->setText(gSavedSettings.getString("VersionChannelName"));
	
	sendChildToBack(getChildView("channel_text"));
	sendChildToBack(getChildView("version_text"));
	sendChildToBack(getChildView("forgot_password_text"));
	
	setDefaultBtn("connect_btn");
	
	// we're important
	setFrontmost(TRUE);
	setFocus(TRUE);
}

LoginFloater::~LoginFloater()
{
	delete LoginFloater::sController;
	
	LoginFloater::sModel = NULL;
	LoginFloater::sController = NULL;
	LoginFloater::sInstance = NULL;
}

void LoginFloater::close()
{
	if(sInstance)
	{
		delete sInstance;
		sInstance = NULL;
	}
}

void LoginFloater::setAlwaysRefresh(bool refresh)
{
	// wargames 2: dead code, LLPanelLogin compatibility
	return;
}

void LoginFloater::refreshLocation( bool force_visible )
{
	if (!sInstance) return;

	LLComboBox* combo = sInstance->getChild<LLComboBox>("start_location_combo");
	
	if (LLURLSimString::parse())
	{
		combo->setCurrentByIndex( 3 );		// BUG?  Maybe 2?
		combo->setTextEntry(LLURLSimString::sInstance.mSimString);
	}
	else
	{
		BOOL login_last = gSavedSettings.getBOOL("LoginLastLocation");
		combo->setCurrentByIndex( login_last ? 1 : 0 );
	}
	
	BOOL show_start = TRUE;
	
	if ( ! force_visible )
		show_start = gSavedSettings.getBOOL("ShowStartLocation");
	
	sInstance->childSetVisible("start_location_combo", show_start);
	sInstance->childSetVisible("start_location_text", show_start);
	sInstance->childSetVisible("server_combo", TRUE);
}

void LoginFloater::newShow(const std::string &grid, bool initialLogin,
						void (*callback)(S32 option, void* user_data), 
						void* callback_data)
{
	if(NULL==sInstance)
	{
		LoginFloater::sGrid = grid;
		LoginFloater::sIsInitialLogin = initialLogin;
		sInstance = new LoginFloater(callback, callback_data);
	}
	
	// floater controller requires initialized floater and model
	if(NULL==sModel)
		sModel = AuthenticationModel::getInstance();
	if(NULL==sController)
		sController = new LoginController(sInstance, sModel, sGrid);
}

void LoginFloater::testShow(void *lies)
{
	// this is if we want to call LoginFloater from a menu option
	// or you know whatever
	newShow(std::string("Test"), false, testCallback, NULL);
}

void LoginFloater::testCallback(S32 option, void *user_data)
{
	// test callback, referenced by testShow()
	if(LOGIN_OPTION_CONNECT == option)
	{
		llinfos << "this is how we connect to a METAVERSE" << llendl;
		std::string first, last, password;
		BOOL remember = TRUE;
		getFields(first, last, password, remember);
		llinfos << "first\t\tlast\t\tpassword" << llendl;
		llinfos << first << "\t\t" << last << "\t\t" << password << llendl;
	}
	else if(LOGIN_OPTION_QUIT == option)
	{
		llinfos << "my login, she die" << llendl;
		llinfos << ":(" << llendl;
		close();
	}
}

void LoginFloater::show(const LLRect &rect, BOOL show_server, 
						void (*callback)(S32 option, void* user_data), 
						void* callback_data)
{
	// we don't need a grid passed in because this is old-style login
	std::string grid = "";
	newShow(grid, TRUE, callback, callback_data);
}

void LoginFloater::setFocus(BOOL b)
{
	if(b != hasFocus())
	{
		if(b)
		{
			LoginFloater::giveFocus();
		}
		else
		{
			LLPanel::setFocus(b);
		}
	}
}

void LoginFloater::giveFocus()
{
	LLComboBox *combo = NULL;
	
	if(NULL==sInstance)
	{
		llwarns << "giveFocus has no LoginFloater instance" << llendl;
		return;
	}
	
	// for our combo box approach, selecting the combo box is almost always
	// the right thing to do on the floater receiving focus
	combo = sInstance->getChild<LLComboBox>("name_combo");
	combo->setFocus(TRUE);
}

void LoginFloater::getFields(std::string &firstname, std::string &lastname, std::string &password,
							 BOOL &remember)
{
	if (!sInstance)
	{
		llwarns << "Attempted getFields with no login view shown" << llendl;
		return;
	}
	
	std::string loginname = sInstance->childGetText("name_combo");
	
	LLStringUtil::replaceTabsWithSpaces(loginname, 1);
	LLStringUtil::trim(loginname);
	std::vector<std::string> loginVec;
	boost::split(loginVec, loginname, boost::is_any_of(" "), boost::token_compress_on);
	if(loginVec.size() == 2)
	{
		firstname = loginVec[0];
		lastname = loginVec[1];
	}
	
	password = sInstance->mMungedPassword;
	remember = sInstance->childGetValue("remember_check");
}

void LoginFloater::getFields(std::string &loginname, std::string &password, BOOL &remember)
{
	std::string first, last, pass;
	BOOL rem;
	getFields(first, last, pass, rem);
	loginname = first + " " + last;
	password = pass;
	remember = rem;
}

void LoginFloater::setFields(const std::string& firstname, const std::string& lastname, const std::string& password,
							 BOOL remember)
{
	if (!sInstance)
	{
		llwarns << "Attempted setFields with no login view shown" << llendl;
		return;
	}
	
	std::string loginname = firstname + " " + lastname;
	sInstance->childSetText("name_combo", loginname);
	
	// Max "actual" password length is 16 characters.
	// Hex digests are always 32 characters.
	if (password.length() == 32)
	{
		// This is a MD5 hex digest of a password.
		// We don't actually use the password input field, 
		// fill it with MAX_PASSWORD characters so we get a 
		// nice row of asterixes.
		const std::string filler("123456789!123456");
		sInstance->childSetText("password_edit", filler);
		sInstance->mIncomingPassword = filler;
		sInstance->mMungedPassword = password;
	}
	else
	{
		// this is a normal text password
		sInstance->childSetText("password_edit", password);
		sInstance->mIncomingPassword = password;
		LLMD5 pass((unsigned char *)password.c_str());
		char munged_password[MD5HEX_STR_SIZE];
		pass.hex_digest(munged_password);
		sInstance->mMungedPassword = munged_password;
	}
	
	sInstance->childSetValue("remember_check", remember);
}

void LoginFloater::setFields(const std::string &loginname, const std::string &password, BOOL remember)
{
	std::vector<std::string> loginVec;
	boost::split(loginVec, loginname, boost::is_any_of(" "), boost::token_compress_on);
	setFields(loginVec[0], loginVec[1], password, remember);
}

BOOL LoginFloater::isGridComboDirty()
{
	BOOL user_picked = FALSE;
	if (!sInstance)
	{
		llwarns << "Attempted getServer with no login view shown" << llendl;
	}
	else
	{
		LLComboBox* combo = sInstance->getChild<LLComboBox>("server_combo");
		user_picked = combo->isDirty();
	}
	return user_picked;
}

void LoginFloater::getLocation(std::string &location)
{
	if (!sInstance)
	{
		llwarns << "Attempted getLocation with no login view shown" << llendl;
		return;
	}
	
	LLComboBox* combo = sInstance->getChild<LLComboBox>("start_location_combo");
	location = combo->getValue().asString();
}

std::string& LoginFloater::getPassword()
{
	return mMungedPassword;
}

void LoginFloater::setPassword(std::string &password)
{
	mMungedPassword = password;
}

bool LoginFloater::isSamePassword(std::string &password)
{
	return mMungedPassword == password;
}

void LoginFloater::addServer(const std::string& server, S32 domain_name)
{
	if (!sInstance)
	{
		llwarns << "Attempted addServer with no login view shown" << llendl;
		return;
	}
	
	LLComboBox* combo = sInstance->getChild<LLComboBox>("server_combo");
	combo->add(server, LLSD(domain_name) );
	combo->setCurrentByIndex(0);
}

void LoginFloater::accept()
{
	if(NULL==sInstance || NULL==sInstance->mCallback)
		return;
	
	sInstance->setFocus(FALSE);
	
	std::string name_combo = sInstance->childGetText("name_combo");
	if(!name_combo.empty())
	{
		sInstance->mCallback(LOGIN_OPTION_CONNECT, sInstance->mCallbackData);
	}
	else
	{
		// TODO: new account call goes here
		return;
	}
}

void LoginFloater::cancel()
{
	if(NULL==sInstance)
		return;
	
	if(sInstance->sIsInitialLogin)
	{
		// send a callback that indicates we're quitting or closing
		if(sInstance->mCallback)
			sInstance->mCallback(LOGIN_OPTION_QUIT, sInstance->mCallbackData);
		return;
	}
	
	sInstance->close();
}
