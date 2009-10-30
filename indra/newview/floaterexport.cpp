
#include "llviewerprecompiledheaders.h"
#include "llviewermenu.h" 


// system library includes
#include <iostream>
#include <fstream>
#include <sstream>

// linden library includes
#include "llfilepicker.h"
#include "indra_constants.h"
#include "llsdserialize.h"
#include "llsdutil.h"

#include "llcallbacklist.h"

// newview includes
#include "llagent.h"
#include "llselectmgr.h"
#include "lltoolplacer.h"

#include "lltexturecache.h"

#include "llnotify.h"

#include "llapr.h"
#include "lldir.h"
#include "llimage.h"
#include "lllfsthread.h"
#include "llviewercontrol.h"
#include "llassetuploadresponders.h"
#include "lleconomy.h"
#include "llhttpclient.h"
#include "lluploaddialog.h"
#include "lldir.h"
#include "llinventorymodel.h"	// gInventory
#include "llviewercontrol.h"	// gSavedSettings
#include "llviewermenu.h"	// gMenuHolder
#include "llagent.h"
#include "llfilepicker.h"
#include "llfloateranimpreview.h"
#include "llfloaterbuycurrency.h"
#include "llfloaterimagepreview.h"
#include "llfloaternamedesc.h"
#include "llfloatersnapshot.h"
#include "llinventorymodel.h"	// gInventory
#include "llresourcedata.h"
#include "llstatusbar.h"
#include "llviewercontrol.h"	// gSavedSettings
#include "llviewerimagelist.h"
#include "lluictrlfactory.h"
#include "llviewermenu.h"	// gMenuHolder
#include "llviewerregion.h"
#include "llviewerstats.h"
#include "llviewerwindow.h"
#include "llappviewer.h"
#include "lluploaddialog.h"
// Included to allow LLTextureCache::purgeTextures() to pause watchdog timeout
#include "llappviewer.h" 
#include "lltransactiontypes.h"

#include "floaterexport.h" 

#include "llviewerobjectlist.h"

//The above includes need thinning.... here is a tested list: -Patrick Sapinski (Tuesday, September 29, 2009)

#include "lllogchat.h"


FloaterExport* FloaterExport::sInstance = 0;

class importResponder: public LLNewAgentInventoryResponder
{
	public:

	importResponder(const LLSD& post_data,
		const LLUUID& vfile_id,
		LLAssetType::EType asset_type)
	: LLNewAgentInventoryResponder(post_data, vfile_id, asset_type)
	{
	}


	//virtual 
	virtual void uploadComplete(const LLSD& content)
	{
			lldebugs << "LLNewAgentInventoryResponder::result from capabilities" << llendl;

	LLAssetType::EType asset_type = LLAssetType::lookup(mPostData["asset_type"].asString());
	LLInventoryType::EType inventory_type = LLInventoryType::lookup(mPostData["inventory_type"].asString());

	// Update L$ and ownership credit information
	// since it probably changed on the server
	if (asset_type == LLAssetType::AT_TEXTURE ||
		asset_type == LLAssetType::AT_SOUND ||
		asset_type == LLAssetType::AT_ANIMATION)
	{
		gMessageSystem->newMessageFast(_PREHASH_MoneyBalanceRequest);
		gMessageSystem->nextBlockFast(_PREHASH_AgentData);
		gMessageSystem->addUUIDFast(_PREHASH_AgentID, gAgent.getID());
		gMessageSystem->addUUIDFast(_PREHASH_SessionID, gAgent.getSessionID());
		gMessageSystem->nextBlockFast(_PREHASH_MoneyData);
		gMessageSystem->addUUIDFast(_PREHASH_TransactionID, LLUUID::null );
		gAgent.sendReliableMessage();

//		LLStringUtil::format_map_t args;
//		args["[AMOUNT]"] = llformat("%d",LLGlobalEconomy::Singleton::getInstance()->getPriceUpload());
//		LLNotifyBox::showXml("UploadPayment", args);
	}

	// Actually add the upload to viewer inventory
	llinfos << "Adding " << content["new_inventory_item"].asUUID() << " "
			<< content["new_asset"].asUUID() << " to inventory." << llendl;
	if(mPostData["folder_id"].asUUID().notNull())
	{
		LLPermissions perm;
		U32 next_owner_perm;
		perm.init(gAgent.getID(), gAgent.getID(), LLUUID::null, LLUUID::null);
		if (mPostData["inventory_type"].asString() == "snapshot")
		{
			next_owner_perm = PERM_ALL;
		}
		else
		{
			next_owner_perm = PERM_MOVE | PERM_TRANSFER;
		}
		perm.initMasks(PERM_ALL, PERM_ALL, PERM_NONE, PERM_NONE, next_owner_perm);
		S32 creation_date_now = time_corrected();
		LLPointer<LLViewerInventoryItem> item
			= new LLViewerInventoryItem(content["new_inventory_item"].asUUID(),
										mPostData["folder_id"].asUUID(),
										perm,
										content["new_asset"].asUUID(),
										asset_type,
										inventory_type,
										mPostData["name"].asString(),
										mPostData["description"].asString(),
										LLSaleInfo::DEFAULT,
										LLInventoryItem::II_FLAGS_NONE,
										creation_date_now);
		gInventory.updateItem(item);
		gInventory.notifyObservers();
	}
	else
	{
		llwarns << "Can't find a folder to put it in" << llendl;
	}

	// remove the "Uploading..." message
	LLUploadDialog::modalUploadFinished();
	
	FloaterExport::getInstance()->update_map(content["new_asset"].asUUID());
	//FloaterExport::getInstance()->upload_next_asset();

	}

};



class CacheReadResponder : public LLTextureCache::ReadResponder
	{
	public:
		CacheReadResponder(const LLUUID& id, LLImageFormatted* image)
			:  mFormattedImage(image), mID(id)
		{
			setImage(image);
		}
		void setData(U8* data, S32 datasize, S32 imagesize, S32 imageformat, BOOL imagelocal)
		{
			if(imageformat==IMG_CODEC_TGA && mFormattedImage->getCodec()==IMG_CODEC_J2C)
			{
				llwarns<<"Bleh its a tga not saving"<<llendl;
				mFormattedImage=NULL;
				mImageSize=0;
				return;
			}

			if (mFormattedImage.notNull())
			{	
				llassert_always(mFormattedImage->getCodec() == imageformat);
				mFormattedImage->appendData(data, datasize);
			}
			else
			{
				mFormattedImage = LLImageFormatted::createFromType(imageformat);
				mFormattedImage->setData(data,datasize);
			}
			mImageSize = imagesize;
			mImageLocal = imagelocal;
		}

		virtual void completed(bool success)
		{
			if(success && (mFormattedImage.notNull()) && mImageSize>0)
			{
				
				llinfos << "SUCCESS getting texture "<<mID<< llendl;
				
				std::string name;
				mID.toString(name);
				llinfos << "Saving to "<<(FloaterExport::getInstance()->getfolder()+"//"+name)<<llendl;
				
				if(gSavedSettings.getBOOL("ExportTGATextures"))
				{
					LLFile::mkdir(FloaterExport::getInstance()->getfolder()+"//textures//");

					LLPointer<LLImageTGA> image_tga = new LLImageTGA;
					LLPointer<LLImageRaw> raw = new LLImageRaw;
					
					mFormattedImage->decode(raw, 0);

					if( !image_tga->encode( raw ) )
					{
						LLStringUtil::format_map_t args;
						args["[FILE]"] = name;
						gViewerWindow->alertXml("CannotEncodeFile", args);
					}
					else if( !image_tga->save( FloaterExport::getInstance()->getfolder()+"//textures//"+name + ".tga" ) )
					{
						LLStringUtil::format_map_t args;
						args["[FILE]"] = name;
						gViewerWindow->alertXml("CannotWriteFile", args);
					}
				}

				if(gSavedSettings.getBOOL("ExportJ2CTextures"))
				{
					if(!mFormattedImage->save(FloaterExport::getInstance()->getfolder()+"//textures//"+name + ".j2c"))
					{
						llinfos << "FAIL saving texture "<<mID<< llendl;
					}
				}

			}
			else
			{
				if(!success)
					llwarns << "FAIL NOT SUCCESSFUL getting texture "<<mID<< llendl;
				if(mFormattedImage.isNull())
					llwarns << "FAIL image is NULL "<<mID<< llendl;
			}	

			FloaterExport::getInstance()->m_nexttextureready=true;
			//JUST SAY NO TO APR DEADLOCKING
			//FloaterExport::getInstance()->export_next_texture();
		}
	private:
		LLPointer<LLImageFormatted> mFormattedImage;
		LLUUID mID;
	};



FloaterExport::FloaterExport()
:	LLFloater( std::string("Prim Export Floater") )
{
	LLUICtrlFactory::getInstance()->buildFloater( this, "floater_prim_export.xml" );

	childSetAction("export", onClickExport, this);
	childSetAction("close", onClickClose, this);

	LLBBox bbox = LLSelectMgr::getInstance()->getBBoxOfSelection();
	LLVector3 box_center_agent = bbox.getCenterAgent();
	//LLVector3 box_corner_agent = bbox.localToAgent( unitVectorToLocalBBoxExtent( partToUnitVector( mManipPart ), bbox ) );
	//mGridScale = mSavedSelectionBBox.getExtentLocal() * 0.5f;
	
	LLVector3 temp = bbox.getExtentLocal();

	std::stringstream sstr;	
	LLUICtrl * ctrl=this->getChild<LLUICtrl>("selection size");	

	sstr <<"X: "<<llformat("%.2f", temp.mV[VX]);
	sstr <<", Y: "<<llformat("%.2f", temp.mV[VY]);
	sstr <<", Z: "<<llformat("%.2f", temp.mV[VZ]);
	//sstr << " \n ";
	//		temp = bbox.getCenterAgent();
	//sstr << "X: "<<temp.mV[VX]<<", Y: "<<temp.mV[VY]<<", Z: "<<temp.mV[VZ];

	ctrl->setValue(LLSD("Text")=sstr.str());
	
	// reposition floater from saved settings
	LLRect rect = gSavedSettings.getRect( "FloaterPrimExport" );
	reshape( rect.getWidth(), rect.getHeight(), FALSE );
	setRect( rect );

	running=false;
	textures.clear();
	assetmap.clear();
	current_asset=LLUUID::null;
	m_retexture=false;
}

FloaterExport* FloaterExport::getInstance()
{
    if ( ! sInstance )
        sInstance = new FloaterExport();

	return sInstance;
}

FloaterExport::~FloaterExport()
{
	// save position of floater
	gSavedSettings.setRect( "FloaterPrimExport", getRect() );

	//which one??
	FloaterExport::sInstance = NULL;
	sInstance = NULL;
}

void FloaterExport::close()
{
	if(sInstance)
	{
		delete sInstance;
		sInstance = NULL;
	}
}

void FloaterExport::draw()
{
	LLFloater::draw();
}

void FloaterExport::onClose( bool app_quitting )
{
	setVisible( false );
	// HACK for fast XML iteration replace with:
	// destroy();
}

void FloaterExport::updateexportnumbers()
{
	std::stringstream sstr;	
	LLUICtrl * ctrl=this->getChild<LLUICtrl>("name_label");	

	if (textures.size() != 0)
	{
		sstr<<"Export Progress \n";
		sstr << "Remaining Textures: "<<textures.size()<<"\n";
	}
	else
		sstr<<"Export Complete.";

	ctrl->setValue(LLSD("Text")=sstr.str());
}

void FloaterExport::pre_export_object()
{
	textures.clear();
	llsd.clear();
	xml = new LLXMLNode("group", FALSE);
	this_group.clear();


	LLUICtrl * ctrl=this->getChild<LLUICtrl>("name_label");	
	ctrl->setValue(LLSD("Text")="");

	if(NULL==sInstance) 
	{
		sInstance = new FloaterExport();
		llwarns << "sInstance assigned. sInstance=" << sInstance << llendl;
	}
	
	sInstance->open();	/*Flawfinder: ignore*/
}


// static
void FloaterExport::onClickExport(void* data)
{
	FloaterExport* self = (FloaterExport*)data;

	// Open the file save dialog
	LLFilePicker& file_picker = LLFilePicker::instance();
	if( !file_picker.getSaveFile( LLFilePicker::FFSAVE_XML ) )
	{
		// User canceled save.
		return;
	}
	 
	self->file_name = file_picker.getCurFile();
	self->folder = gDirUtilp->getDirName(self->file_name);

	self->export_state=EXPORT_INIT;
	gIdleCallbacks.addFunction(exportworker, NULL);
}

// static
void FloaterExport::onClickClose(void* data)
{
	sInstance->close();
}

void FloaterExport::exportworker(void *userdata)
{	
	FloaterExport::sInstance->updateexportnumbers();

	switch(FloaterExport::sInstance->export_state)
	{
		case EXPORT_INIT:
			{	
				LLSelectMgr::getInstance()->getSelection()->ref();

				struct ff : public LLSelectedNodeFunctor
					{
						virtual bool apply(LLSelectNode* node)
						{
							if(gAgent.getID()!=node->mPermissions->getOwner())
							{
								#ifdef LL_GRID_PERMISSIONS
									return false;
								#else
									return true;
								#endif
							}
							else if(581632==node->mPermissions->getMaskOwner() || 2147483647==node->mPermissions->getMaskOwner())
							{
								return true;
							}
							return false;
						}
				} func;

				if(LLSelectMgr::getInstance()->getSelection()->applyToNodes(&func,false))
					FloaterExport::sInstance->export_state=EXPORT_STRUCTURE;
				else
				{
					llwarns<<"Incorrect permission to export"<<llendl;
					FloaterExport::sInstance->export_state=EXPORT_DONE;
					gIdleCallbacks.deleteFunction(exportworker);
					LLSelectMgr::getInstance()->getSelection()->unref();

				}
				break;
			}

			break;
		case EXPORT_STRUCTURE:
		{
			struct ff : public LLSelectedObjectFunctor
			{
				virtual bool apply(LLViewerObject* object)
				{
					object->boostTexturePriority(TRUE);
					LLViewerObject::child_list_t children = object->getChildren();
					children.push_front(object); //push root onto list
 					
					LLXMLNodePtr max_xml = FloaterExport::sInstance->xml->createChild("max", FALSE);
					LLVector3 max = object->getPosition();
					max_xml->createChild("x", TRUE)->setValue(llformat("%.5f", max.mV[VX]));
					max_xml->createChild("y", TRUE)->setValue(llformat("%.5f", max.mV[VY]));
					max_xml->createChild("z", TRUE)->setValue(llformat("%.5f", max.mV[VZ]));

					LLXMLNodePtr min_xml = FloaterExport::sInstance->xml->createChild("min", FALSE);
					LLVector3 min = object->getPosition();
					min_xml->createChild("x", TRUE)->setValue(llformat("%.5f", min.mV[VX]));
					min_xml->createChild("y", TRUE)->setValue(llformat("%.5f", min.mV[VY]));
					min_xml->createChild("z", TRUE)->setValue(llformat("%.5f", min.mV[VZ]));
					
					LLXMLNodePtr center_xml = FloaterExport::sInstance->xml->createChild("center", FALSE);
					LLVector3 center = object->getPosition();
					center_xml->createChild("x", TRUE)->setValue(llformat("%.5f", center.mV[VX]));
					center_xml->createChild("y", TRUE)->setValue(llformat("%.5f", center.mV[VY]));
					center_xml->createChild("z", TRUE)->setValue(llformat("%.5f", center.mV[VZ]));

					FloaterExport::sInstance->xml->addChild(FloaterExport::sInstance->prims_to_xml(children));
					return true;
				}
			} func;

			FloaterExport::sInstance->export_state=EXPORT_LLSD;
			LLSelectMgr::getInstance()->getSelection()->applyToRootObjects(&func,false);
			LLSelectMgr::getInstance()->getSelection()->unref();

			break;
		}
		case EXPORT_TEXTURES:
			if(FloaterExport::sInstance->m_nexttextureready==false)
				return;

			//Ok we got work to do
			FloaterExport::sInstance->m_nexttextureready=false;

			if(FloaterExport::sInstance->textures.empty())
			{
				FloaterExport::sInstance->export_state=EXPORT_DONE;
				return;
			}

			FloaterExport::sInstance->export_next_texture();
		break;

		case EXPORT_LLSD:
			{
				// Create a file stream and write to it
				llofstream export_file(FloaterExport::sInstance->file_name);
				LLSDSerialize::toPrettyXML(FloaterExport::sInstance->llsd, export_file);
				export_file.close();

				llofstream out(FloaterExport::sInstance->file_name);
				
				if (!out.good())
				{
					llwarns << "Unable to open for output." << llendl;
				}

				out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
				
				LLXMLNode *temp_xml = new LLXMLNode("project", FALSE);
				temp_xml->createChild("schema", FALSE)->setValue("1.0");
				temp_xml->createChild("name", FALSE)->setValue(FloaterExport::sInstance->file_name);
				temp_xml->createChild("date", FALSE)->setValue(LLLogChat::timestamp(1));
				temp_xml->createChild("sotware", FALSE)->setValue(llformat("%s %d.%d.%d.%d",
				LLAppViewer::instance()->getSecondLifeTitle().c_str(), LL_VERSION_MAJOR, LL_VERSION_MINOR, LL_VERSION_PATCH, LL_VERSION_BUILD));
				temp_xml->createChild("platform", FALSE)->setValue("Second Life");
				temp_xml->createChild("grid", FALSE)->setValue("test");
				
				temp_xml->addChild(FloaterExport::sInstance->xml);

				temp_xml->writeToOstream(out);
				out.close();

				FloaterExport::sInstance->m_nexttextureready=true;	
				FloaterExport::sInstance->export_state=EXPORT_TEXTURES;
			}
			break;
		case EXPORT_DONE:
					llinfos<<"Backup complete"<<llendl
					gIdleCallbacks.deleteFunction(exportworker);
					//FloaterExport::getInstance()->close();
					//sInstance->close();

			break;
	}
}

LLXMLNode *FloaterExport::prims_to_xml(LLViewerObject::child_list_t child_list)
{

	LLViewerObject* object;
	LLXMLNode *xml = new LLXMLNode("linkset", FALSE);

	char localid[16];

	for (LLViewerObject::child_list_t::iterator i = child_list.begin(); i != child_list.end(); ++i)
	{
		object=(*i);
		LLUUID id = object->getID();

		llinfos << "Exporting prim " << object->getID().asString() << llendl;
				
		std::string selected_item	= "box";
		F32 scale_x=1.f, scale_y=1.f;
		
		const LLVolumeParams &volume_params = object->getVolume()->getParams();

		// Volume type
		U8 path = volume_params.getPathParams().getCurveType();
		U8 profile_and_hole = volume_params.getProfileParams().getCurveType();
		U8 profile	= profile_and_hole & LL_PCODE_PROFILE_MASK;
		U8 hole		= profile_and_hole & LL_PCODE_HOLE_MASK;
		
		// Scale goes first so we can differentiate between a sphere and a torus,
		// which have the same profile and path types.

		// Scale
		scale_x = volume_params.getRatioX();
		scale_y = volume_params.getRatioY();

		BOOL linear_path = (path == LL_PCODE_PATH_LINE) || (path == LL_PCODE_PATH_FLEXIBLE);
		if ( linear_path && profile == LL_PCODE_PROFILE_CIRCLE )
		{
			selected_item = "cylinder";
		}
		else if ( linear_path && profile == LL_PCODE_PROFILE_SQUARE )
		{
			selected_item = "box";
		}
		else if ( linear_path && profile == LL_PCODE_PROFILE_ISOTRI )
		{
			selected_item = "prism";
		}
		else if ( linear_path && profile == LL_PCODE_PROFILE_EQUALTRI )
		{
			selected_item = "prism";
		}
		else if ( linear_path && profile == LL_PCODE_PROFILE_RIGHTTRI )
		{
			selected_item = "prism";
		}
		else if (path == LL_PCODE_PATH_FLEXIBLE) // shouldn't happen
		{
			selected_item = "cylinder"; // reasonable default
		}
		else if ( path == LL_PCODE_PATH_CIRCLE && profile == LL_PCODE_PROFILE_CIRCLE && scale_y > 0.75f)
		{
			selected_item = "sphere";
		}
		else if ( path == LL_PCODE_PATH_CIRCLE && profile == LL_PCODE_PROFILE_CIRCLE && scale_y <= 0.75f)
		{
			selected_item = "torus";
		}
		else if ( path == LL_PCODE_PATH_CIRCLE && profile == LL_PCODE_PROFILE_CIRCLE_HALF)
		{
			selected_item = "sphere";
		}
		else if ( path == LL_PCODE_PATH_CIRCLE2 && profile == LL_PCODE_PROFILE_CIRCLE )
		{
			// Spirals aren't supported.  Make it into a sphere.  JC
			selected_item = "sphere";
		}
		else if ( path == LL_PCODE_PATH_CIRCLE && profile == LL_PCODE_PROFILE_EQUALTRI )
		{
			selected_item = "ring";
		}
		else if ( path == LL_PCODE_PATH_CIRCLE && profile == LL_PCODE_PROFILE_SQUARE && scale_y <= 0.75f)
		{
			selected_item = "tube";
		}
		else
		{
			llinfos << "Unknown path " << (S32) path << " profile " << (S32) profile << " in getState" << llendl;
			selected_item = "box";
		}

		if (object->getParameterEntryInUse(LLNetworkData::PARAMS_SCULPT))
		{
			selected_item = "sculpt";
		}

		// Create an LLSD object that represents this prim. It will be injected in to the overall LLSD
		// tree structure
		LLXMLNode *prim_xml = new LLXMLNode(selected_item.c_str(), FALSE);

		if (!object->isRoot())
		{

			// Parent id
			snprintf(localid, sizeof(localid), "%u", object->getSubParent()->getLocalID());
 			prim_xml->createChild("uuid", FALSE)->setValue(localid);
		}


		LLSelectNode* node = LLSelectMgr::getInstance()->getSelection()->findNode(object);
		if (node)
		{
 			prim_xml->createChild("name", FALSE)->setValue("<![CDATA[" + node->mName + "]]>");
 			prim_xml->createChild("description", FALSE)->setValue("<![CDATA[" + node->mDescription + "]]>");
		}

		// Transforms		
		LLXMLNodePtr position_xml = prim_xml->createChild("position", FALSE);
		LLVector3 position;
		position.setVec(object->getPositionRegion());
		position_xml->createChild("x", TRUE)->setValue(llformat("%.5f", position.mV[VX]));
		position_xml->createChild("y", TRUE)->setValue(llformat("%.5f", position.mV[VY]));
		position_xml->createChild("z", TRUE)->setValue(llformat("%.5f", position.mV[VZ]));

		LLXMLNodePtr scale_xml = prim_xml->createChild("size", FALSE);
		LLVector3 scale = object->getScale();
		scale_xml->createChild("x", TRUE)->setValue(llformat("%.5f", scale.mV[VX]));
		scale_xml->createChild("y", TRUE)->setValue(llformat("%.5f", scale.mV[VY]));
		scale_xml->createChild("z", TRUE)->setValue(llformat("%.5f", scale.mV[VZ]));

		LLXMLNodePtr rotation_xml = prim_xml->createChild("rotation", FALSE);
		LLQuaternion rotation = object->getRotationEdit();
		rotation_xml->createChild("x", TRUE)->setValue(llformat("%.5f", rotation.mQ[VX]));
		rotation_xml->createChild("y", TRUE)->setValue(llformat("%.5f", rotation.mQ[VY]));
		rotation_xml->createChild("z", TRUE)->setValue(llformat("%.5f", rotation.mQ[VZ]));
		rotation_xml->createChild("w", TRUE)->setValue(llformat("%.5f", rotation.mQ[VW]));

		// Flags
		if(object->flagPhantom())
		{
			LLXMLNodePtr shadow_xml = prim_xml->createChild("phantom", FALSE);
			shadow_xml->createChild("val", TRUE)->setValue("true");
		}

		if(object->mFlags & FLAGS_USE_PHYSICS)
		{
			LLXMLNodePtr shadow_xml = prim_xml->createChild("physical", FALSE);
			shadow_xml->createChild("val", TRUE)->setValue("true");
		}
		
		// Grab S path
		F32 begin_s	= volume_params.getBeginS();
		F32 end_s	= volume_params.getEndS();

		// Compute cut and advanced cut from S and T
		F32 begin_t = volume_params.getBeginT();
		F32 end_t	= volume_params.getEndT();

		// Hollowness
		F32 hollow = volume_params.getHollow();

		// Twist
		F32 twist		= volume_params.getTwist() * 180.0;
		F32 twist_begin = volume_params.getTwistBegin() * 180.0;

		// Cut interpretation varies based on base object type
		F32 cut_begin, cut_end, adv_cut_begin, adv_cut_end;

		if ( selected_item == "sphere" || selected_item == "torus" || 
			 selected_item == "tube"   || selected_item == "ring" )
		{
			cut_begin		= begin_t;
			cut_end			= end_t;
			adv_cut_begin	= begin_s;
			adv_cut_end		= end_s;
		}
		else
		{
			cut_begin       = begin_s;
			cut_end         = end_s;
			adv_cut_begin   = begin_t;
			adv_cut_end     = end_t;
		}

		if (selected_item != "sphere")
		{		
			// Shear
			//<top_shear x="0" y="0" />
			F32 shear_x = volume_params.getShearX();
			F32 shear_y = volume_params.getShearY();
			LLXMLNodePtr shear_xml = prim_xml->createChild("top_shear", FALSE);
			shear_xml->createChild("x", TRUE)->setValue(llformat("%.5f", shear_x));
			shear_xml->createChild("y", TRUE)->setValue(llformat("%.5f", shear_y));
		
		
		}

		if (selected_item == "box" || selected_item == "cylinder" || selected_item == "prism")
		{
			// Taper
			//<taper x="0" y="0" />
			F32 taper_x = 1.f - volume_params.getRatioX();
			F32 taper_y = 1.f - volume_params.getRatioY();
			LLXMLNodePtr taper_xml = prim_xml->createChild("taper", FALSE);
			taper_xml->createChild("x", TRUE)->setValue(llformat("%.5f", taper_x));
			taper_xml->createChild("y", TRUE)->setValue(llformat("%.5f", taper_y));
		}
		else if (selected_item == "torus" || selected_item == "tube" || selected_item == "ring")
		{
			// Taper
			//<taper x="0" y="0" />
			F32 taper_x	= volume_params.getTaperX();
			F32 taper_y = volume_params.getTaperY();
			LLXMLNodePtr taper_xml = prim_xml->createChild("taper", FALSE);
			taper_xml->createChild("x", TRUE)->setValue(llformat("%.5f", taper_x));
			taper_xml->createChild("y", TRUE)->setValue(llformat("%.5f", taper_y));


			//Hole Size
			//<hole_size x="0.2" y="0.35" />
			F32 hole_size_x = volume_params.getRatioX();
			F32 hole_size_y = volume_params.getRatioY();
			LLXMLNodePtr hole_size_xml = prim_xml->createChild("hole_size", FALSE);
			hole_size_xml->createChild("x", TRUE)->setValue(llformat("%.5f", hole_size_x));
			hole_size_xml->createChild("y", TRUE)->setValue(llformat("%.5f", hole_size_y));


			//Advanced cut
			//<profile_cut begin="0" end="1" />
			LLXMLNodePtr profile_cut_xml = prim_xml->createChild("profile_cut", FALSE);
			profile_cut_xml->createChild("begin", TRUE)->setValue(llformat("%.5f", adv_cut_begin));
			profile_cut_xml->createChild("end", TRUE)->setValue(llformat("%.5f", adv_cut_end));


		}

		//<path_cut begin="0" end="1" />
		LLXMLNodePtr path_cut_xml = prim_xml->createChild("path_cut", FALSE);
		path_cut_xml->createChild("begin", TRUE)->setValue(llformat("%.5f", cut_begin));
		path_cut_xml->createChild("end", TRUE)->setValue(llformat("%.5f", cut_end));

		//<twist begin="0" end="0" />
		LLXMLNodePtr twist_xml = prim_xml->createChild("twist", FALSE);
		twist_xml->createChild("begin", TRUE)->setValue(llformat("%.5f", twist_begin));
		twist_xml->createChild("end", TRUE)->setValue(llformat("%.5f", twist));


		// All hollow objects allow a shape to be selected.
		if (hollow > 0.f)
		{
			const char	*selected_hole	= "1";

			switch (hole)
			{
			case LL_PCODE_HOLE_CIRCLE:
				selected_hole = "3";
				break;
			case LL_PCODE_HOLE_SQUARE:
				selected_hole = "2";
				break;
			case LL_PCODE_HOLE_TRIANGLE:
				selected_hole = "4";
				break;
			case LL_PCODE_HOLE_SAME:
			default:
				selected_hole = "1";
				break;
			}

			//<hollow amount="0" shape="1" />
			LLXMLNodePtr hollow_xml = prim_xml->createChild("hollow", FALSE);
			hollow_xml->createChild("amount", TRUE)->setValue(llformat("%.5f", hollow * 100.0));
			hollow_xml->createChild("shape", TRUE)->setValue(llformat("%s", selected_hole));
		}

		// Extra params // b6fab961-af18-77f8-cf08-f021377a7244

		// Flexible
		if (object->isFlexible())
		{  //FIXME
			LLFlexibleObjectData* flex = (LLFlexibleObjectData*)object->getParameterEntry(LLNetworkData::PARAMS_FLEXIBLE);
			prim_xml->createChild("flexible", FALSE)->setValue(flex->asLLSD());
		}
		
		// Light
		if (object->getParameterEntryInUse(LLNetworkData::PARAMS_LIGHT))
		{
			LLLightParams* light = (LLLightParams*)object->getParameterEntry(LLNetworkData::PARAMS_LIGHT);

			//<light>
			LLXMLNodePtr light_xml = prim_xml->createChild("light", FALSE);

			//<color r="255" g="255" b="255" />
			LLXMLNodePtr color_xml = light_xml->createChild("color", FALSE);
			LLColor4 color = light->getColor();
			color_xml->createChild("r", TRUE)->setValue(llformat("%u", color.mV[VRED]));
			color_xml->createChild("g", TRUE)->setValue(llformat("%u", color.mV[VGREEN]));
			color_xml->createChild("b", TRUE)->setValue(llformat("%u", color.mV[VBLUE]));

			//<intensity val="1.0" />
			LLXMLNodePtr intensity_xml = light_xml->createChild("intensity", FALSE);
			intensity_xml->createChild("val", TRUE)->setValue(llformat("%.5f", color.mV[VALPHA]));

			//<radius val="10.0" />
			LLXMLNodePtr radius_xml = light_xml->createChild("radius", FALSE);
			radius_xml->createChild("val", TRUE)->setValue(llformat("%.5f", light->getRadius()));

			//<falloff val="0.75" />
			LLXMLNodePtr falloff_xml = light_xml->createChild("falloff", FALSE);
			falloff_xml->createChild("val", TRUE)->setValue(llformat("%.5f", light->getFalloff()));

			//return light->getCutoff(); wtf is this?
		}

		// Sculpt
		if (object->getParameterEntryInUse(LLNetworkData::PARAMS_SCULPT))
		{
			LLSculptParams* sculpt = (LLSculptParams*)object->getParameterEntry(LLNetworkData::PARAMS_SCULPT);

			//<topology val="4" />
			LLXMLNodePtr topology_xml = prim_xml->createChild("topology", FALSE);
			topology_xml->createChild("val", TRUE)->setValue(llformat("%u", sculpt->getSculptType()));
			
			//<sculptmap_uuid>1e366544-c287-4fff-ba3e-5fafdba10272</sculptmap_uuid>
			//<sculptmap_file>apple_map.tga</sculptmap_file>
			//FIXME png/tga/j2c selection itt.
			prim_xml->createChild("sculptmap_file", FALSE)->setValue(llformat("%s", "testing"));
			prim_xml->createChild("sculptmap_uuid", FALSE)->setValue(llformat("%s", "testing"));

			//prim_xml->createChild("sculptmap_file", FALSE)->setValue(llformat("%s", sculpt->getSculptTexture()));
			//prim_xml->createChild("sculptmap_uuid", FALSE)->setValue(llformat("%s", sculpt->getSculptTexture()));

			LLUUID sculpt_texture=sculpt->getSculptTexture();
			bool alreadyseen=false;
			std::list<LLUUID>::iterator iter;
			for(iter = textures.begin(); iter != textures.end() ; iter++) 
			{
				if( (*iter)==sculpt_texture)
					alreadyseen=true;
			}
			if(alreadyseen==false)
			{
				llinfos << "Found a sculpt texture, adding to list "<<sculpt_texture<<llendl;
				textures.push_back(sculpt_texture);
			}
		}

		//<texture>
		LLXMLNodePtr texture_xml = prim_xml->createChild("texture", FALSE);

		// Textures
		LLSD te_llsd;
		U8 te_count = object->getNumTEs();
		for (U8 i = 0; i < te_count; i++)
		{
			bool alreadyseen=false;
			te_llsd.append(object->getTE(i)->asLLSD());
			std::list<LLUUID>::iterator iter;
			for(iter = textures.begin(); iter != textures.end() ; iter++) 
			{
				if( (*iter)==object->getTE(i)->getID())
					alreadyseen=true;
			}

			//<face id=0>
			LLXMLNodePtr face_xml = texture_xml->createChild("face", FALSE);
			//This may be a hack, but it's ok since we're not using id in this code. We set id differently because for whatever reason
			//llxmlnode filters a few parameters including ID. -Patrick Sapinski (Friday, September 25, 2009)
			face_xml->mID = llformat("%d", i);

			//<tile u="-1" v="1" />
			//object->getTE(face)->mScaleS
			//object->getTE(face)->mScaleT
			LLXMLNodePtr tile_xml = face_xml->createChild("tile", FALSE);
			tile_xml->createChild("u", TRUE)->setValue(llformat("%.5f", object->getTE(i)->mScaleS));
			tile_xml->createChild("v", TRUE)->setValue(llformat("%.5f", object->getTE(i)->mScaleT));

			//<offset u="0" v="0" />
			//object->getTE(face)->mOffsetS
			//object->getTE(face)->mOffsetT
			LLXMLNodePtr offset_xml = face_xml->createChild("offset", FALSE);
			offset_xml->createChild("u", TRUE)->setValue(llformat("%.5f", object->getTE(i)->mOffsetS));
			offset_xml->createChild("v", TRUE)->setValue(llformat("%.5f", object->getTE(i)->mOffsetT));


			//<rotation w="0" />
			//object->getTE(face)->mRotation
			LLXMLNodePtr rotation_xml = face_xml->createChild("rotation", FALSE);
			rotation_xml->createChild("w", TRUE)->setValue(llformat("%.5f", object->getTE(i)->mRotation));


			//<image_file><![CDATA[76a0319a-e963-44b0-9f25-127815226d71.tga]]></image_file>
			//<image_uuid>76a0319a-e963-44b0-9f25-127815226d71</image_uuid>
			LLUUID texture = object->getTE(i)->getID();
			std::string uuid_string;
			object->getTE(i)->getID().toString(uuid_string);
			
			face_xml->createChild("image_file", FALSE)->setValue("<![CDATA[" + uuid_string + ".tga]]>");
			face_xml->createChild("image_uuid", FALSE)->setValue(uuid_string);


			//<color r="255" g="255" b="255" />
			LLXMLNodePtr color_xml = face_xml->createChild("color", FALSE);
			LLColor4 color = object->getTE(i)->getColor();
			color_xml->createChild("r", TRUE)->setValue(llformat("%.5f", color.mV[VRED]));
			color_xml->createChild("g", TRUE)->setValue(llformat("%.5f", color.mV[VGREEN]));
			color_xml->createChild("b", TRUE)->setValue(llformat("%.5f", color.mV[VBLUE]));

			//<transparency val="0" />
			LLXMLNodePtr transparency_xml = face_xml->createChild("transparency", FALSE);
			transparency_xml->createChild("val", TRUE)->setValue(llformat("%.5f", color.mV[VALPHA]));

			//<glow val="0" />
			//object->getTE(face)->getGlow()
			LLXMLNodePtr glow_xml = face_xml->createChild("glow", FALSE);
			glow_xml->createChild("val", TRUE)->setValue(llformat("%.5f", object->getTE(i)->getGlow()));

			//<fullbright val="false" />
			//object->getTE(face)->getFullbright()
			if(object->getTE(i)->getFullbright())
			{
				LLXMLNodePtr fullbright_xml = face_xml->createChild("fullbright", FALSE);
				fullbright_xml->createChild("val", TRUE)->setValue("true");
			}
				
			//<shine val="0" />
			//object->getTE(face)->getShiny()
			if (object->getTE(i)->getShiny())
			{
				LLXMLNodePtr shine_xml = face_xml->createChild("shine", FALSE);
				shine_xml->createChild("val", TRUE)->setValue("1");
			}
				
			//<bump val="0" />
			//object->getTE(face)->getBumpmap()
			if (object->getTE(i)->getBumpmap())
			{
				LLXMLNodePtr bumpmap_xml = face_xml->createChild("bumpmap", FALSE);
				bumpmap_xml->createChild("val", TRUE)->setValue("1");
			}
				
			//<mapping val="0" />

			if(alreadyseen==false)
				textures.push_back(object->getTE(i)->getID());
		}

		// The keys in the primitive maps do not have to be localids, they can be any
		// string. We simply use localids because they are a unique identifier
		snprintf(localid, sizeof(localid), "%u", object->getLocalID());
		//put prim_xml inside of xml? -Patrick Sapinski (Thursday, September 17, 2009)
		//llsd[(const char*)localid] = prim_llsd;
		xml->addChild(prim_xml);

	}

	updateexportnumbers();

	return xml;
}
/* old export code
LLSD FloaterExport::prims_to_llsd(LLViewerObject::child_list_t child_list)
{

	LLViewerObject* object;
	LLSD llsd;

	char localid[16];

	for (LLViewerObject::child_list_t::iterator i = child_list.begin(); i != child_list.end(); ++i)
	{
		object=(*i);
		LLUUID id = object->getID();

		llinfos << "Exporting prim " << object->getID().asString() << llendl;

		// Create an LLSD object that represents this prim. It will be injected in to the overall LLSD
		// tree structure
		LLSD prim_llsd;

		if (!object->isRoot())
		{

			// Parent id
			snprintf(localid, sizeof(localid), "%u", object->getSubParent()->getLocalID());
			prim_llsd["parent"] = localid;
		}

		// Transforms
		prim_llsd["position"] = object->getPosition().getValue();
		prim_llsd["scale"] = object->getScale().getValue();
		prim_llsd["rotation"] = ll_sd_from_quaternion(object->getRotation());

		// Flags
		prim_llsd["shadows"] = object->flagCastShadows();
		prim_llsd["phantom"] = object->flagPhantom();
		prim_llsd["physical"] = (BOOL)(object->mFlags & FLAGS_USE_PHYSICS);

		// Volume params
		LLVolumeParams params = object->getVolume()->getParams();
		prim_llsd["volume"] = params.asLLSD();

		// Extra paramsb6fab961-af18-77f8-cf08-f021377a7244
		if (object->isFlexible())
		{
			// Flexible
			LLFlexibleObjectData* flex = (LLFlexibleObjectData*)object->getParameterEntry(LLNetworkData::PARAMS_FLEXIBLE);
			prim_llsd["flexible"] = flex->asLLSD();
		}
		if (object->getParameterEntryInUse(LLNetworkData::PARAMS_LIGHT))
		{
			// Light
			LLLightParams* light = (LLLightParams*)object->getParameterEntry(LLNetworkData::PARAMS_LIGHT);
			prim_llsd["light"] = light->asLLSD();
		}
		if (object->getParameterEntryInUse(LLNetworkData::PARAMS_SCULPT))
		{
			// Sculpt
			LLSculptParams* sculpt = (LLSculptParams*)object->getParameterEntry(LLNetworkData::PARAMS_SCULPT);
			prim_llsd["sculpt"] = sculpt->asLLSD();
			
			LLUUID sculpt_texture=sculpt->getSculptTexture();
			bool alreadyseen=false;
			std::list<LLUUID>::iterator iter;
			for(iter = textures.begin(); iter != textures.end() ; iter++) 
			{
				if( (*iter)==sculpt_texture)
					alreadyseen=true;
			}
			if(alreadyseen==false)
			{
				llinfos << "Found a sculpt texture, adding to list "<<sculpt_texture<<llendl;
				textures.push_back(sculpt_texture);
			}
		}

		// Textures
		LLSD te_llsd;
		U8 te_count = object->getNumTEs();
		for (U8 i = 0; i < te_count; i++)
		{
			bool alreadyseen=false;
			te_llsd.append(object->getTE(i)->asLLSD());
			std::list<LLUUID>::iterator iter;
			for(iter = textures.begin(); iter != textures.end() ; iter++) 
			{
				if( (*iter)==object->getTE(i)->getID())
					alreadyseen=true;
			}
			if(alreadyseen==false)
				textures.push_back(object->getTE(i)->getID());
		}
		prim_llsd["textures"] = te_llsd;

		// The keys in the primitive maps do not have to be localids, they can be any
		// string. We simply use localids because they are a unique identifier
		snprintf(localid, sizeof(localid), "%u", object->getLocalID());
		llsd[(const char*)localid] = prim_llsd;
	}

	updateexportnumbers();

	return llsd;
}
*/

void FloaterExport::export_next_texture()
{
	if(textures.empty())
	{
		llinfos << "Finished exporting textures "<<llendl;
		return;
	}

	std::list<LLUUID>::iterator iter;
	iter = textures.begin();

	LLUUID id;

	while(1)
	{
		if(iter==textures.end())
		{
			m_nexttextureready=true;
			return;
		}

		id=(*iter);

		LLViewerImage * imagep = gImageList.hasImage(id);
		if(imagep!=NULL)
		{
			S32 cur_discard = imagep->getDiscardLevel();
			if(cur_discard>0)
			{
				if(imagep->getBoostLevel()!=LLViewerImageBoostLevel::BOOST_PREVIEW)
					imagep->setBoostLevel(LLViewerImageBoostLevel::BOOST_PREVIEW); //we want to force discard 0 this one does this.
			}
			else
			{
				break;
			}
		}
		else
		{
			llwarns<<" We *DONT* have the texture "<<llendl;
		}

		//texture download progress:
		F32 data_progress = imagep->mDownloadProgress;
			llwarns<<" download progress "<<data_progress<<llendl;
		
		iter++;
	}

	textures.remove(id);

	llinfos<<"Requesting texture "<<id<<llendl;
	LLImageJ2C * mFormattedImage = new LLImageJ2C;
	CacheReadResponder* responder = new CacheReadResponder(id, mFormattedImage);
  	LLAppViewer::getTextureCache()->readFromCache(id,LLWorkerThread::PRIORITY_HIGH,0,999999,responder);
}

