/** 
 * @file LLGraphicsRemotectrl.cpp
 * @author Patrick Sapinski
 * @brief A remote control for graphics settings
 *
 */

#ifndef LL_LLGRAPHICSREMOTECTRL_H
#define LL_LLGRAPHICSREMOTECTRL_H

#include "llpanel.h"

////////////////////////////////////////////////////////////////////////////////
//
class LLGraphicsRemoteCtrl : public LLPanel
{
public:
	LLGraphicsRemoteCtrl ();
	
	/*virtual*/ ~LLGraphicsRemoteCtrl ();
	/*virtual*/ BOOL postBuild();
	/*virtual*/ void draw();

	void enableGraphicsButtons();

	LLPanel* mControls;
	
	static void onClickExpandBtn(void* user_data);
	//static void* createGraphicsPanel(void* data);

	/// handle if WL preset is changed
	static void onChangePresetName(LLUICtrl* ctrl, void* userData);
	
	virtual void setToolTip(const std::string& msg);

protected:
	void build();
};

#endif
