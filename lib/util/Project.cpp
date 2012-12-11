#include <sys/param.h>
#include <iostream>

#include <libxml/parser.h>
#include <libxml/xmlreader.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>
#include <hrpModel/Config.h>
#include "Project.h"

Project::Project() : 
    m_timeStep(0.001), m_logTimeStep(0.01), m_totalTime(1.0), m_gravity(9.8), m_isEuler(true), m_kinematicsOnly(false)
{
}

bool Project::parse(const std::string& filename)
{
  xmlInitParser();
  xmlDocPtr doc = xmlParseFile(filename.c_str());
  if ( doc == NULL ) {
      std::cerr << "unable to parse file(" << filename << ")" << std::endl;
      return false;
  }

  /* Create xpath evaluation context */
  xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
  if ( xpathCtx == NULL ) {
      std::cerr << "unable to create new XPath context" << std::endl;
      xmlFreeDoc(doc);
      return false;
  }

  /* Evaluate xpath expression */
  xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(BAD_CAST "/grxui/mode/item", xpathCtx);
  if ( xmlXPathNodeSetIsEmpty(xpathObj->nodesetval) ) {
      std::cerr << "unable to find <mode>" << std::endl;
  }

  int size;
  size = xpathObj->nodesetval->nodeNr;

  for (int i = 0; i < size; i++ ) {
      xmlNodePtr node = xpathObj->nodesetval->nodeTab[i];
      //std::cerr << i << " class:" << xmlGetProp(node, (xmlChar *)"class") << std::endl;
      if ( xmlStrEqual( xmlGetProp(node, (xmlChar *)"class"), (xmlChar *)"com.generalrobotix.ui.item.GrxSimulationItem")  ) {
          xmlNodePtr cur_node = node->children;
          while ( cur_node ) {
              if ( cur_node->type == XML_ELEMENT_NODE ) {
                  if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"integrate") ) {
                      m_kinematicsOnly = std::string((char *)(xmlGetProp(cur_node, (xmlChar *)"value"))) == "false";
                      
                  } else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"viewsimulate") ) {
                  } else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"totalTime") ) {
                      m_totalTime = atof((char *)(xmlGetProp(cur_node, (xmlChar *)"value")));
                  } else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"timeStep") ) {
                      m_timeStep = atof((char *)(xmlGetProp(cur_node, (xmlChar *)"value")));
                  } else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"realTime") ) {
                  } else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"gravity") ) {
                      m_gravity = atof((char *)(xmlGetProp(cur_node, (xmlChar *)"value")));
                  } else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"method") ) {
                      m_isEuler = std::string((char *)(xmlGetProp(cur_node, (xmlChar *)"value"))) == std::string("EULER");
                  } else {
#if 0
                      std::cerr << "Unknown tag : " << cur_node->name << " "
                                << "name=" << xmlGetProp(cur_node, (xmlChar *)"name")
                                << "value=" << xmlGetProp(cur_node, (xmlChar *)"value") << std::endl;
#endif
                  }
              }
              cur_node = cur_node->next;
          }
      } else if ( xmlStrEqual( xmlGetProp(node, (xmlChar *)"class"), (xmlChar *)"com.generalrobotix.ui.item.GrxRTSItem")  ) {
          xmlNodePtr cur_node = node->children;
          while ( cur_node ) {
              if ( cur_node->type == XML_ELEMENT_NODE ) {
                  std::string name = (char *)xmlGetProp(cur_node, (xmlChar *)"name");
                  std::string value = (char *)xmlGetProp(cur_node, (xmlChar *)"value");
                  //std::cout << name << "," << value << std::endl;

                  if (name == "connection"){
                      int pos = value.find(':');
                      if (pos < 0){
                          std::cerr << "can't find a separator(:) in " 
                                    << value << std::endl; 
                      }else{
                          std::string p1 = value.substr(0,pos);
                          std::string p2 = value.substr(pos+1);
                          m_rts.connections.push_back(std::make_pair(p1, p2));
                      }
                  }else{
                      int pos = name.find('.');
                      if (pos < 0){
                          std::cerr << "unknown property name:" << name 
                                    << std::endl; 
                      }else{
                          std::string comp = name.substr(0,pos);
                          std::string cat = name.substr(pos+1);
                          if (cat == "factory"){
                              m_rts.components[comp].path = value;
                              int pos = value.find_last_of("/");
                              m_rts.components[comp].name = value.substr(pos+1);
                          }else if (cat == "period") {
                              m_rts.components[comp].period = atof(value.c_str());
                          }
                      }
                  }
              }
              cur_node = cur_node->next;
          }
      } else if ( xmlStrEqual( xmlGetProp(node, (xmlChar *)"class"), (xmlChar *)"com.generalrobotix.ui.item.GrxModelItem")  ) {
          //std::cerr << "GrxModelItem name:" << xmlGetProp(node, (xmlChar *)"name") << ", url:" << xmlGetProp(node, (xmlChar *)"url") << std::endl;
          std::string path = (char *)xmlGetProp(node, (xmlChar *)"url");
          if ( path.find("$(CURRENT_DIR)") != std::string::npos ) {
              if (filename.find_last_of("/") != std::string::npos){
                  path.replace(path.find("$(CURRENT_DIR)"),14, 
                               filename.substr(0, filename.find_last_of("/")));
              }else{
                  path.replace(path.find("$(CURRENT_DIR)"),15, ""); 
              }
              if (path[0] != '/'){
                  char buf[MAXPATHLEN];
                  path = std::string(getcwd(buf, MAXPATHLEN))+"/"+path;
              }
          }
          if ( path.find("$(PROJECT_DIR)") != std::string::npos ) {
              std::string shdir = OPENHRP_SHARE_DIR;
              std::string pjdir = shdir + "/sample/project";
              path.replace(path.find("$(PROJECT_DIR)"),14, pjdir);
          }
          ModelItem m;
          m.url = std::string("file://")+path;
          xmlNodePtr cur_node = node->children;
          while ( cur_node ) {
              if ( cur_node->type == XML_ELEMENT_NODE ) {
                  if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"isRobot") ) {
                      if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"value"),(xmlChar *)"true")) {
                          //isRobot = true;
                      }
                  } else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"controlTime") ) {
                      //controlTimeStep = atof((char *)(xmlGetProp(cur_node, (xmlChar *)"value")));
                  } else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"rtcName") ) {
                      m.rtcName = (char *)xmlGetProp(cur_node, (xmlChar *)"value");
                  } else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"inport") ) {
                      m.inports.push_back((char *)xmlGetProp(cur_node, (xmlChar *)"value"));
                  } else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"outport") ) {
                      m.outports.push_back((char *)xmlGetProp(cur_node, (xmlChar *)"value"));
                  } else if ( std::string((char *)xmlGetProp(cur_node, (xmlChar *)"name")).rfind(".angle") != std::string::npos ) {
                      std::string name = std::string((char *)xmlGetProp(cur_node, (xmlChar *)"name"));
                      name.erase(name.rfind(".angle"));
                      m.joint[name].angle = atof((char *)xmlGetProp(cur_node, (xmlChar *)"value"));
                  } else if ( std::string((char *)xmlGetProp(cur_node, (xmlChar *)"name")).rfind(".mode") != std::string::npos ) {
                      std::string name = std::string((char *)xmlGetProp(cur_node, (xmlChar *)"name"));
                      name.erase(name.rfind(".mode"));
                      m.joint[name].isHighGain = xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"value"), (xmlChar *)"HighGain");
                  } else if ( std::string((char *)xmlGetProp(cur_node, (xmlChar *)"name")).rfind(".NumOfAABB") != std::string::npos ) {
                      std::string name = std::string((char *)xmlGetProp(cur_node, (xmlChar *)"name"));
                      name.erase(name.rfind(".NumOfAABB"));
                      m.joint[name].NumOfAABB = atof((char *)(xmlGetProp(cur_node, (xmlChar *)"value")));
                  } else if ( std::string((char *)xmlGetProp(cur_node, (xmlChar *)"name")).rfind(".translation") != std::string::npos ) {
                      std::string name = std::string((char *)xmlGetProp(cur_node, (xmlChar *)"name"));
                      name.erase(name.rfind(".translation"));
                      float x, y, z;
                      sscanf(((char *)xmlGetProp(cur_node, (xmlChar *)"value")),"%f %f %f", &x, &y, &z);
                      m.joint[name].translation[0] = x; m.joint[name].translation[1] = y; m.joint[name].translation[2] = z;
                  } else if ( std::string((char *)xmlGetProp(cur_node, (xmlChar *)"name")).rfind(".rotation") != std::string::npos ) {
                      std::string name = std::string((char *)xmlGetProp(cur_node, (xmlChar *)"name"));
                      name.erase(name.rfind(".rotation"));
                      float x, y, z, w;
                      sscanf(((char *)xmlGetProp(cur_node, (xmlChar *)"value")),"%f %f %f %f", &x, &y, &z, &w);
                      hrp::calcRodrigues(m.joint[name].rotation, hrp::Vector3(x, y, z), w);
                  } else {
#if 0
                      std::cerr << "Unknown tag : " << cur_node->name << " "
                                << "name=" << xmlGetProp(cur_node, (xmlChar *)"name") << " "
                                << "value=" << xmlGetProp(cur_node, (xmlChar *)"value") << std::endl;
#endif
                  }
              }
              cur_node = cur_node->next;
          }
          std::string n = std::string((char *)xmlGetProp(node, (xmlChar *)"name"));
          m_models[n] = m;
      } else if ( xmlStrEqual( xmlGetProp(node, (xmlChar *)"class"), (xmlChar *)"com.generalrobotix.ui.item.GrxWorldStateItem") ) {
          xmlNodePtr cur_node = node->children;
          while ( cur_node ) {
              if ( cur_node->type == XML_ELEMENT_NODE ) {
                  if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"logTimeStep") ) {
                      m_logTimeStep = atof((char *)(xmlGetProp(cur_node, (xmlChar *)"value")));
                  } else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"timeStep") ) {
                      m_timeStep = atof((char *)(xmlGetProp(cur_node, (xmlChar *)"value")));
                  } else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"totalTime") ) {
                      m_totalTime = atof((char *)(xmlGetProp(cur_node, (xmlChar *)"value")));
                  } else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"method") ) {
                      m_isEuler = std::string((char *)(xmlGetProp(cur_node, (xmlChar *)"value"))) == std::string("EULER");
                  }
              }
              cur_node = cur_node->next;
          }
      } else if ( xmlStrEqual ( xmlGetProp(node, (xmlChar *)"class"), (xmlChar *)"com.generalrobotix.ui.item.GrxCollisionPairItem")  ) {
          CollisionPairItem c;
          xmlNodePtr cur_node = node->children;
          while ( cur_node ) {
              if ( cur_node->type == XML_ELEMENT_NODE ) {
                  if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"objectName1") ) {
                      c.objectName1 = (char *)(xmlGetProp(cur_node, (xmlChar *)"value"));
                  } else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"objectName2") ) {
                      c.objectName2 = (char *)(xmlGetProp(cur_node, (xmlChar *)"value"));
                  } else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"jointName1") ) {
                      c.jointName1 = (char *)(xmlGetProp(cur_node, (xmlChar *)"value"));
                  } else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"jointName2") ) {
                      c.jointName2 = (char *)(xmlGetProp(cur_node, (xmlChar *)"value"));
                  } else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"slidingFriction") ) {
                      c.slidingFriction = atof((char *)(xmlGetProp(cur_node, (xmlChar *)"value")));
                  } else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"staticFriction") ) {
                      c.staticFriction = atof((char *)(xmlGetProp(cur_node, (xmlChar *)"value")));
                  } else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"cullingThresh") ) {
                      c.cullingThresh = atof((char *)(xmlGetProp(cur_node, (xmlChar *)"value")));
                  } else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"sprintDamperModel") ) {
                      c.sprintDamperModel = (char *)(xmlGetProp(cur_node, (xmlChar *)"value"));
                  } else {
#if 0
                      std::cerr << "Unknown tag : " << cur_node->name << " "
                                << ", name=" << xmlGetProp(cur_node, (xmlChar *)"name")
                                << ", value=" << xmlGetProp(cur_node, (xmlChar *)"value") << std::endl;
#endif
                  }
              }
              cur_node = cur_node->next;
          }
          m_collisionPairs.push_back(c);
      }
  }

  /* Cleanup Xpath Data */
  xmlXPathFreeObject(xpathObj);
  xmlXPathFreeContext(xpathCtx);

  {
  /* Create xpath evaluation context */
  xmlXPathContextPtr xpathCtx = xmlXPathNewContext(doc);
  if ( xpathCtx == NULL ) {
      std::cerr << "unable to create new XPath context" << std::endl;
      xmlFreeDoc(doc);
      return false;
  }

  xmlXPathObjectPtr xpathObj = xmlXPathEvalExpression(BAD_CAST "/grxui/mode/view", xpathCtx);
  if ( xmlXPathNodeSetIsEmpty(xpathObj->nodesetval) ) {
      std::cerr << "unable to find <mode>" << std::endl;
  }

  int size;
  size = xpathObj->nodesetval->nodeNr;

  for (int i = 0; i < size; i++ ) {
      xmlNodePtr node = xpathObj->nodesetval->nodeTab[i];
      //std::cerr << i << " class:" << xmlGetProp(node, (xmlChar *)"class") << std::endl;
      if ( xmlStrEqual( xmlGetProp(node, (xmlChar *)"class"), (xmlChar *)"com.generalrobotix.ui.view.GrxRobotHardwareClientView")  ) {
          xmlNodePtr cur_node = node->children;
          while ( cur_node ) {
              if ( cur_node->type == XML_ELEMENT_NODE ) {
                  if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"robotHost") ) {
                      m_rhview.hostname = (char *)(xmlGetProp(cur_node, (xmlChar *)"value"));
                  }else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"robotPort") ) {
                      m_rhview.port = atoi((char *)(xmlGetProp(cur_node, (xmlChar *)"value")));
                  }else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"interval") ) {
                      m_rhview.interval = atoi((char *)(xmlGetProp(cur_node, (xmlChar *)"value")));
                  }else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"stateProvider") ) {
                      m_rhview.RobotHardwareName = (char *)(xmlGetProp(cur_node, (xmlChar *)"value"));
                  }else if ( xmlStrEqual(xmlGetProp(cur_node, (xmlChar *)"name"),(xmlChar *)"StateHolderRTC") ) {
                      m_rhview.StateHolderName = (char *)(xmlGetProp(cur_node, (xmlChar *)"value"));
                  }
              }
              cur_node = cur_node->next;
          }
      }
  }
  xmlXPathFreeObject(xpathObj);
  xmlXPathFreeContext(xpathCtx);
  }

  /* free the document */
  xmlFreeDoc(doc);
  xmlCleanupParser();

  return true;
}

