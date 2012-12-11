#include <fstream>
#include <boost/bind.hpp>
#include <boost/function.hpp>
#include <rtm/Manager.h>
#include <rtm/CorbaNaming.h>
#include <hrpModel/ModelLoaderUtil.h>
#ifdef __APPLE__
#include <GLUT/glut.h>
#else
#include <GL/glut.h>
#endif
#include <SDL_thread.h>
#include "util/GLbodyRTC.h"
#include "util/GLlink.h"
#include "util/GLutil.h"
#include "util/Project.h"
#include "util/OpenRTMUtil.h"
#include "util/SDLUtil.h"
#include "util/BVutil.h"
#include "Simulator.h"
#include "GLscene.h"

using namespace std;
using namespace hrp;
using namespace OpenHRP;

hrp::BodyPtr createBody(const std::string& name, const ModelItem& mitem,
                        ModelLoader_ptr modelloader, GLscene *scene,
                        bool usebbox)
{
    std::cout << "createBody(" << name << "," << mitem.url << ")" << std::endl;
    RTC::Manager& manager = RTC::Manager::instance();
    std::string args = "GLbodyRTC?instance_name="+name;
    GLbodyRTC *glbodyrtc = (GLbodyRTC *)manager.createComponent(args.c_str());
    hrp::BodyPtr body = hrp::BodyPtr(glbodyrtc);
    BodyInfo_var binfo;
    try{
        OpenHRP::ModelLoader::ModelLoadOption opt;
        opt.readImage = true;
        opt.AABBdata.length(0);
        opt.AABBtype = OpenHRP::ModelLoader::AABB_NUM;
        binfo = modelloader->getBodyInfoEx(mitem.url.c_str(), opt);
    }catch(OpenHRP::ModelLoader::ModelLoaderException ex){
        std::cerr << ex.description << std::endl;
        return hrp::BodyPtr();
    }
    if (!loadBodyFromBodyInfo(body, binfo, true, GLlinkFactory)){
        std::cerr << "failed to load model[" << mitem.url << "]" << std::endl;
        manager.deleteComponent(glbodyrtc);
        return hrp::BodyPtr();
    }else{
        if (usebbox) convertToAABB(body);
        for (std::map<std::string, JointItem>::const_iterator it2=mitem.joint.begin();
             it2 != mitem.joint.end(); it2++){
            hrp::Link *link = body->link(it2->first);
            if (link) link->isHighGainMode = it2->second.isHighGain;
        }
        for (size_t i=0; i<mitem.inports.size(); i++){
            glbodyrtc->createInPort(mitem.inports[i]);
        }
        for (size_t i=0; i<mitem.outports.size(); i++){
            glbodyrtc->createOutPort(mitem.outports[i]);
        }
        loadShapeFromBodyInfo(glbodyrtc, binfo);
        body->setName(name);
        scene->addBody(body);
        return body;
    }
}

int main(int argc, char* argv[]) 
{
    bool display = true, realtime=false, usebbox=false, endless=false;
    bool showsensors = false;
    int wsize = 0;
    bool exitOnFinish = false;
    bool record = false;

    if (argc < 0){
        std::cerr << "Usage:" << argv[0] << " [project file] [options]"
                  << std::endl;
        return 1;
    }

    Project prj;
    if (!prj.parse(argv[1])){
        std::cerr << "failed to parse " << argv[1] << std::endl;
        return 1;
    }

    for (int i=2; i<argc; i++){
        if (strcmp("-nodisplay",argv[i])==0){
            display = false;
        }else if(strcmp("-realtime", argv[i])==0){
            realtime = true;
        }else if(strcmp("-usebbox", argv[i])==0){
            usebbox = true;
        }else if(strcmp("-endless", argv[i])==0){
            endless = true;
        }else if(strcmp("-showsensors", argv[i])==0){
            showsensors = true;
        }else if(strcmp("-size", argv[i])==0){
            wsize = atoi(argv[++i]);
        }else if(strcmp("-exit-on-finish", argv[i])==0){
            exitOnFinish = true;
        }else if(strcmp("-record", argv[i])==0){
            record = true;
            exitOnFinish = true;
        }
    }

    //================= OpenRTM =========================
    RTC::Manager* manager;
    int rtmargc=0;
    std::vector<char *> rtmargv;
    for (int i=1; i<argc; i++){
        if (strcmp(argv[i], "-nodisplay") 
            && strcmp(argv[i], "-realtime")
            && strcmp(argv[i], "-usebbox")
            && strcmp(argv[i], "-endless")
            && strcmp(argv[i], "-showsensors")
            && strcmp(argv[i], "-size")
            && strcmp(argv[i], "-exit-on-finish")
            && strcmp(argv[i], "-record")
            ){
            rtmargv.push_back(argv[i]);
            rtmargc++;
        }
    }
    manager = RTC::Manager::init(rtmargc, rtmargv.data());
    manager->init(rtmargc, rtmargv.data());
    GLbodyRTC::moduleInit(manager);
    manager->activateManager();
    manager->runManager(true);

    std::string nameServer = manager->getConfig()["corba.nameservers"];
    int comPos = nameServer.find(",");
    if (comPos < 0){
        comPos = nameServer.length();
    }
    nameServer = nameServer.substr(0, comPos);
    RTC::CorbaNaming naming(manager->getORB(), nameServer.c_str());

    ModelLoader_var modelloader = getModelLoader(CosNaming::NamingContext::_duplicate(naming.getRootContext()));
    //==================== Viewer setup ===============
    LogManager<SceneState> log;
    GLscene scene(&log);
    scene.showSensors(showsensors);
    Simulator simulator(&log);

    SDLwindow window(&scene, &log, &simulator);
    if (display) window.init(wsize, wsize);

    //================= setup Simulator ======================
    BodyFactory factory = boost::bind(createBody, _1, _2, modelloader, &scene, usebbox);
    simulator.init(prj, factory);
    simulator.realTime(realtime);
    if (endless){
        simulator.setTotalTime(0);
        log.enableRingBuffer(50000);
    }

    std::cout << "timestep = " << prj.timeStep() << ", total time = " 
              << prj.totalTime() << std::endl;

    if (display){
        simulator.start();
        while(window.oneStep()){
            if (exitOnFinish && !simulator.isRunning()) break;
        };
        simulator.stop();
        if (record){
            log.record();
            while(window.oneStep()){
                if (!log.isRecording()) break;
            }
        }
    }else{
        while (simulator.oneStep());
    }

    manager->shutdown();

    return 0;
}
