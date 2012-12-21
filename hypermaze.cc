#define IRRLICHT
#include "irrlicht.h"
#include "irrdisp.hh"
#include "mazegen.hh"
#include "vector.hh"
#include "iMyCamera.hh"
#include "dirns.hh"
#include "gui.hh"
#include "keymap.hh"
#include "helpgui.hh"
#include <fstream>
#include <map>

using namespace std;

namespace irr{
  using namespace core;
  using namespace scene;
  using namespace io;
  using namespace video;
};


class MyEventReceiver : public irr::IEventReceiver{


  private:
    // We use this array to store the current state of each key
    bool actionTriggered[KeyMap::A_COUNT];

  public:
    
    KeyMap map;
  
    bool MouseDown;
    irr::position2d<irr::s32> MousePos;
    
    // This is the one method that we have to implement
    virtual bool OnEvent(const irr::SEvent& event)
    {
      // Remember whether each key is down or up
      if (event.EventType == irr::EET_KEY_INPUT_EVENT && event.KeyInput.PressedDown){
        KeyMap::Action a;
        if(event.KeyInput.Char==0)
          a=map.getAction(event.KeyInput.Key);
        else
          a=map.getAction(event.KeyInput.Char);
        if(a!=KeyMap::A_NONE){
          actionTriggered[a]=true;
          return true;
        }
      }

      if (event.EventType != irr::EET_MOUSE_INPUT_EVENT)
        return false;

      switch(event.MouseInput.Event){
        case irr::EMIE_LMOUSE_PRESSED_DOWN:
          MouseDown = true;
          break;
        case irr::EMIE_LMOUSE_LEFT_UP:
          MouseDown = false;
          break;
        case irr::EMIE_MOUSE_MOVED:
          MousePos = irr::position2d<irr::s32> (event.MouseInput.X,event.MouseInput.Y);
          break;
      }
      return false;
    }

    // This is used to check whether a key is being held down
    virtual bool isTriggered(KeyMap::Action a)
    {
      if(actionTriggered[a]){
        actionTriggered[a]=false;
        return true;
       }
       return false;
    }
    
    MyEventReceiver():MouseDown(false),MousePos(0,0)
    {
      for (irr::u32 i=0; i<KeyMap::A_COUNT; ++i)
        actionTriggered[i] = false;
    }

};

class MyNodeGen:public NodeGen{
  irr::ISceneManager* smgr;
  irr::ITexture* wall;
  irr::ITexture* string;
  irr::ITexture* activeString;
  irr::ITexture* handle;
  public:
    MyNodeGen(irr::ISceneManager* smgr,irr::ITexture* wall,irr::ITexture* string,irr::ITexture* activeString,irr::ITexture* handle):
        smgr(smgr),wall(wall),string(string),activeString(activeString),handle(handle){};
    virtual irr::IMeshSceneNode* makeUnitWall(bool isNode){
      irr::IMeshSceneNode* node = smgr->addCubeSceneNode(1);
      node->setMaterialTexture( 0, wall);
      node->setMaterialFlag(irr::video::EMF_LIGHTING, true);
      return node;
    }

    virtual irr::IMeshSceneNode* makeUnitString(bool isNode){
      irr::IMeshSceneNode* node = smgr->addCubeSceneNode(1);
      node->setMaterialTexture( 0, string);
      node->setMaterialFlag(irr::video::EMF_LIGHTING, true);
      return node;
    }

    virtual void makeStringActive(irr::IMeshSceneNode* node,bool active){
      if(active)
        node->setMaterialTexture(0,activeString);
      else
        node->setMaterialTexture(0,string);
    }
    virtual irr::IMeshSceneNode* makeUnitHandle(){
      irr::IMeshSceneNode* node = smgr->addSphereSceneNode(1);
      node->setMaterialType(irr::video::EMT_TRANSPARENT_ALPHA_CHANNEL);
      node->setMaterialTexture( 0, handle);
      node->setMaterialFlag(irr::video::EMF_LIGHTING, true);
      irr::ITriangleSelector* selector = smgr->createTriangleSelector(node->getMesh(),node);
      node->setTriangleSelector(selector);
      selector->drop(); // We're done with this selector, so drop it now.
      return node;
    };

};


int main(){

  MyEventReceiver e;
  e.map.addMapping(irr::KEY_F1,KeyMap::A_CONF);
  wifstream ifs("hypermaze.keymap.conf");
  ifs>>e.map;
  ifs.close();
  
	irr::IrrlichtDevice *device =
		irr::createDevice( irr::video::EDT_OPENGL, irr::dimension2d<irr::u32>(640, 480), 16,
			false, false, false, &e);

	if (!device)
		return 1;
	
	device->setResizable(true);
	
  device->getGUIEnvironment()->getSkin()->setFont(device->getGUIEnvironment()->getFont("irrlicht/myfont.xml"));
  irr::SColor c=device->getGUIEnvironment()->getSkin()->getColor(irr::EGDC_3D_HIGH_LIGHT);
  c.setAlpha(170);
  device->getGUIEnvironment()->getSkin()->setColor(irr::EGDC_3D_HIGH_LIGHT,c);
	
	device->setWindowCaption(L"Hyper Maze! - Irrlicht");
	irr::IVideoDriver* driver = device->getVideoDriver();
	irr::ISceneManager* smgr = device->getSceneManager();
	
	addCameraSceneNodeMy(smgr,device->getCursorControl(),0,irr::vector3df(0,0,0),
	    irr::vector3df(0,0,-300),-500.,-100.,3.,100.);
	
  irr::ILightSceneNode* light1 =
                smgr->addLightSceneNode(0, irr::vector3df(-100,100,-100),
                irr::video::SColorf(1.0f,1.0f,1.0f,1.0f), 800.0f);
  smgr->setAmbientLight(irr::video::SColorf(0.3,0.3,0.3,1));

  MyNodeGen* ng=new MyNodeGen(smgr,driver->getTexture("irrlicht/wall.png"),driver->getTexture("irrlicht/string.png"),driver->getTexture("irrlicht/activeString.png"),driver->getTexture("irrlicht/handle.png"));
  
  PuzzleDisplay pd(ng);
  
  irr::ISceneCollisionManager* collMan = smgr->getSceneCollisionManager();
  irr::ISceneNode* slice=0;
  int sliced=0;
  irr::vector3df sliceStart;
  
  irr::u32 actionTime[KeyMap::A_COUNT];
  for (irr::u32 i=0; i<KeyMap::A_COUNT; ++i)
    actionTime[i] = 0;
  const irr::u32 DELAY=500;
  
  while(device->run())
	{
    const irr::u32 now = device->getTimer()->getTime();
	  
	  if(slice==0&&e.MouseDown){
	    irr::triangle3df tmp;
	    slice=collMan-> getSceneNodeAndCollisionPointFromRay(
	        collMan->getRayFromScreenCoordinates(e.MousePos),sliceStart,tmp);
	    sliced=0;
	  }
	  if(slice){
	    irr::line3d<irr::f32> ray=collMan->getRayFromScreenCoordinates(e.MousePos);
	    irr::vector3df dir=con(to_vector(pd.getSlicers().find(slice)->second));
	    irr::vector3df ldir=ray.getVector();
	    irr::f32 d=(sliceStart-ray.start).dotProduct(dir*ldir.getLengthSQ()-ldir*dir.dotProduct(ldir))/(dir.dotProduct(ldir)*dir.dotProduct(ldir)-dir.getLengthSQ()*ldir.getLengthSQ())/15-sliced;
	    while(d>1&&pd.hideSide(pd.getSlicers().find(slice)->second,false)){
	      d--;
	      sliced++;
	    }
	    while(d<-1&&pd.hideSide(pd.getSlicers().find(slice)->second,true)){
	      d++;
	      sliced--;
	    }
	      
	    if(!e.MouseDown)
	      slice=0;
	  }
	  
    for(const pair<KeyMap::Action,pair<Dirn,bool> >* it=KeyMap::sliceActions;it!=KeyMap::sliceActions+12;++it){
      if(e.isTriggered(it->first)&&actionTime[it->first]<now){
        pd.hideSide(it->second.first,it->second.second);
        actionTime[it->first]=now+1*DELAY;
      }
    }
    for(const pair<KeyMap::Action,pair<bool,bool> >* it=KeyMap::slideActions;it!=KeyMap::slideActions+4;++it){
      if(e.isTriggered(it->first)&&actionTime[it->first]<now){
        if(pd.ss.slide(it->second.first,it->second.second)){
          pd.stringSelectionUpdated();
          actionTime[it->first]=now+1*DELAY;
        }
      }
    }
    for(const pair<KeyMap::Action,Dirn>* it=KeyMap::moveActions;it!=KeyMap::moveActions+6;++it){
      if(e.isTriggered(it->first)&&actionTime[it->first]<now){
        if(pd.ss.tryMove(it->second)){
          pd.stringUpdated();
          actionTime[it->first]=now+1*DELAY;
        }
      }
    }

    if(e.isTriggered(KeyMap::A_GENERATE)&&actionTime[KeyMap::A_GENERATE]<now){
      GenerateGui gg;
      if(gg.generate(device,pd.m)){
        pd.mazeUpdated();
      }
      actionTime[KeyMap::A_GENERATE]=now+1*DELAY;
    }
    if(e.isTriggered(KeyMap::A_LOAD)&&actionTime[KeyMap::A_LOAD]<now){
      OpenGui og;
      if(og.open(device,pd.m)){
        pd.mazeUpdated();
      }
      actionTime[KeyMap::A_GENERATE]=now+1*DELAY;
    }
    if(e.isTriggered(KeyMap::A_SAVE)&&actionTime[KeyMap::A_SAVE]<now){
      SaveGui sg;
      sg.save(device,pd.m);
      actionTime[KeyMap::A_SAVE]=now+1*DELAY;
    }
    if(e.isTriggered(KeyMap::A_CONF)&&actionTime[KeyMap::A_CONF]<now){
      HelpGui kmg;
      kmg.help(device,e.map);
      actionTime[KeyMap::A_CONF]=now+1*DELAY;
    }
	
		driver->beginScene(true, true, irr::SColor(255,100,101,140));

		smgr->drawAll();
		driver->endScene();
	}
	device->drop();

	return 0;
}
