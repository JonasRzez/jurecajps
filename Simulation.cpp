/**
 * \file        Simulation.cpp
 * \date        Dec 15, 2010
 * \version     v0.5
 * \copyright   <2009-2014> Forschungszentrum Jülich GmbH. All rights reserved.
 *
 * \section License
 * This file is part of JuPedSim.
 *
 * JuPedSim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * JuPedSim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with JuPedSim. If not, see <http://www.gnu.org/licenses/>.
 *
 * \section Description
 * The Simulation class represents a simulation of pedestrians
 * based on a certain model in a specific scenario. A simulation is defined by
 * various parameters and functions.
 *
 *
 **/


#include "Simulation.h"

#include "math/GCFMModel.h"
#include "math/GompertzModel.h"
#include "geometry/Goal.h"

#ifdef _OPENMP
#include <omp.h>
#else
#define omp_get_thread_num() 0
#define omp_get_max_threads()  1
#endif


using namespace std;

OutputHandler* Log;

Simulation::Simulation()
{
     _nPeds = 0;
     _tmax = 0;
     _seed=8091983;
     _deltaT = 0;
     _building = NULL;
     _distribution = NULL;
     _direction = NULL;
     _operationalModel = NULL;
     _solver = NULL;
     _iod = new IODispatcher();
     _fps=1;
     _em=NULL;
     _argsParser=NULL;
     _hpc=-1;
     _profiling=false;
}

Simulation::~Simulation()
{
     delete _building;
     delete _distribution;
     delete _direction;
     delete _operationalModel;
     delete _solver;
     delete _iod;
     delete _em;
}

int Simulation::GetPedsNumber() const
{
     return _nPeds;
}

void Simulation::InitArgs(ArgumentParser* args)
{
     char tmp[CLENGTH];
     string s = "Parameter:\n";

     _argsParser=args;
     switch (args->GetLog()) {
     case 0:
          // no log file
          //Log = new OutputHandler();
          break;
     case 1:
          if(Log) delete Log;
          Log = new STDIOHandler();
          break;
     case 2: {
          char name[CLENGTH]="";
          sprintf(name,"%s.P0.dat",args->GetErrorLogFile().c_str());
          if(Log) delete Log;
          Log = new FileHandler(name);
     }
     break;
     default:
          printf("Wrong option for Logfile!\n\n");
          exit(0);
     }


     if(args->GetPort()!=-1) {
          switch(args->GetFileFormat()) {
          case FORMAT_XML_PLAIN_WITH_MESH:
          case FORMAT_XML_PLAIN: {
               OutputHandler* travisto = new SocketHandler(args->GetHostname(), args->GetPort());
               Trajectories* output= new TrajectoriesJPSV06();
               output->SetOutputHandler(travisto);
               _iod->AddIO(output);
               break;
          }
          case FORMAT_XML_BIN: {
               Log->Write("INFO: \tFormat xml-bin not yet supported in streaming\n");
               //exit(0);
               break;
          }
          case FORMAT_PLAIN: {
               Log->Write("INFO: \tFormat plain not yet supported in streaming\n");
               exit(0);
               break;
          }
          case FORMAT_VTK: {
               Log->Write("INFO: \tFormat vtk not yet supported in streaming\n");
               exit(0);
               break;
          }
          }

          s.append("\tonline streaming enabled \n");
     }

     if(args->GetTrajectoriesFile().empty()==false) {
          switch (args->GetFileFormat()) {
          case FORMAT_XML_PLAIN: {
               OutputHandler* tofile = new FileHandler(args->GetTrajectoriesFile().c_str());
               Trajectories* output= new TrajectoriesJPSV05();
               output->SetOutputHandler(tofile);
               _iod->AddIO(output);
               break;
          }
          case FORMAT_PLAIN: {
               OutputHandler* file = new FileHandler(args->GetTrajectoriesFile().c_str());
               Trajectories* output= new  TrajectoriesFLAT();
               output->SetOutputHandler(file);
               _iod->AddIO(output);
               break;
          }
          case FORMAT_VTK: {
               Log->Write("INFO: \tFormat vtk not yet supported\n");
               OutputHandler* file = new FileHandler((args->GetTrajectoriesFile() +".vtk").c_str());
               Trajectories* output= new  TrajectoriesVTK();
               output->SetOutputHandler(file);
               _iod->AddIO(output);
               break;
          }

          case FORMAT_XML_PLAIN_WITH_MESH: {
               //OutputHandler* tofile = new FileHandler(args->GetTrajectoriesFile().c_str());
               //if(_iod) delete _iod;
               //_iod = new TrajectoriesXML_MESH();
               //_iod->AddIO(tofile);
               break;
          }
          case FORMAT_XML_BIN: {
               // OutputHandler* travisto = new SocketHandler(args->GetHostname(), args->GetPort());
               // Trajectories* output= new TrajectoriesJPSV06();
               // output->SetOutputHandler(travisto);
               // _iod->AddIO(output);
               break;
          }
          }
     }

     _distribution = new PedDistributor();
     _distribution->InitDistributor(_argsParser);
     //s.append(_distribution->writeParameter());

     // define how the navigation line is crossed
     int direction = args->GetExitStrategy();
     sprintf(tmp, "\tDirection to the exit: %d\n", direction);
     s.append(tmp);
     switch (direction) {
     case 1:
          _direction = new DirectionMiddlePoint();
          break;
     case 2:
          _direction = new DirectionMinSeperationShorterLine();
          break;
     case 3:
          _direction = new DirectionInRangeBottleneck();
          break;
     case 4:
          _direction = new DirectionGeneral();
          break;
     default:
          _direction = new DirectionMinSeperationShorterLine();
          sprintf(tmp, "\t Warning Direction %d is not in [1,4]. Fall back to 2\n", direction);
          s.append(tmp);
          break;
     }
     int model =  args->GetModel();
     if(model == MODEL_GFCM)
     { //GCFM
          //if(args->GetHPCFlag()==0){
          _operationalModel = new GCFMModel(_direction, args->GetNuPed(), args->GetNuWall(), args->GetDistEffMaxPed(),
                    args->GetDistEffMaxWall(), args->GetIntPWidthPed(), args->GetIntPWidthWall(),
                    args->GetMaxFPed(), args->GetMaxFWall());
          s.append("\tModel: GCFMModel\n");
          s.append(_operationalModel->GetDescription());
          //}
          //else if(args->GetHPCFlag()==2){
          //  _model = new GPU_acc_GCFMModel(_direction, args->GetNuPed(), args->GetNuWall(), args->GetDistEffMaxPed(),
          //        args->GetDistEffMaxWall(), args->GetIntPWidthPed(), args->GetIntPWidthWall(),
          //      args->GetMaxFPed(), args->GetMaxFWall());
          //s.append("\tModel: GCFMModel\n");
          //s.append(_model->writeParameter());
          //}
          //else {
          //  _model = new GPU_ocl_GCFMModel(_direction, args->GetNuPed(), args->GetNuWall(), args->GetDistEffMaxPed(),
          //        args->GetDistEffMaxWall(), args->GetIntPWidthPed(), args->GetIntPWidthWall(),
          //      args->GetMaxFPed(), args->GetMaxFWall());
          //s.append("\tModel: GCFMModel\n");
          //s.append(_model->writeParameter());
          //}
     }
     else if (model == MODEL_GOMPERTZ)
     { //Gompertz
          _operationalModel = new GompertzModel(_direction, args->GetNuPed(), args->GetaPed(), args->GetbPed(), args->GetcPed(),
                    args->GetNuWall(), args->GetaWall(), args->GetbWall(), args->GetcWall() );
          s.append("\tModel: GompertzModel\n");
          s.append(_operationalModel->GetDescription());
     }

     // ODE solver
     int solver = args->GetSolver();
     sprintf(tmp, "\tODE Solver: %d\n", solver);
     s.append(tmp);
     //switch (solver) {
     //case 1:
          //_solver = new EulerSolver(_model);
          //break;
          //case 2:
          //     _solver = new VelocityVerletSolver(_model);
          //     break;
          //case 3:
          //     _solver = new LeapfrogSolver(_model);
          //     break;
     //}
     sprintf(tmp, "\tnCPU: %d\n", args->GetMaxOpenMPThreads());
     s.append(tmp);
     _tmax = args->GetTmax();
     sprintf(tmp, "\tt_max: %f\n", _tmax);
     s.append(tmp);
     _deltaT = args->Getdt();
     sprintf(tmp, "\tdt: %f\n", _deltaT);
     s.append(tmp);

     _fps=args->Getfps();
     sprintf(tmp, "\tfps: %f\n", _fps);
     s.append(tmp);

     // Route choice
     vector< pair<int, RoutingStrategy> >  routers=  args->GetRoutingStrategy();
     RoutingEngine* routingEngine= new RoutingEngine();

     for (unsigned int r= 0; r<routers.size(); r++) {

          RoutingStrategy strategy=routers[r].second;

          int routerID=routers[r].first;

          switch (strategy) {
          case ROUTING_LOCAL_SHORTEST: {
               Router* router=new GlobalRouter();
               router->SetID(routerID);
               router->SetStrategy(strategy);
               routingEngine->AddRouter(router);
               s.append("\tRouting Strategy local shortest added\n");
               break;
          }
          case ROUTING_GLOBAL_SHORTEST: {
               Router* router=new GlobalRouter();
               router->SetID(routerID);
               router->SetStrategy(strategy);
               routingEngine->AddRouter(router);
               s.append("\tRouting Strategy global shortest added\n");
               break;
          }
          case ROUTING_QUICKEST: {
               Router* router=new QuickestPathRouter();
               router->SetID(routerID);
               router->SetStrategy(strategy);
               routingEngine->AddRouter(router);
               s.append("\tRouting Strategy quickest path added\n");
               break;
          }
          case ROUTING_DYNAMIC: {
               Router* router=new GraphRouter();
               router->SetID(routerID);
               router->SetStrategy(strategy);
               routingEngine->AddRouter(router);
               s.append("\tRouting Strategy graph router added\n");
               break;
          }
          case ROUTING_NAV_MESH: {
               Router* router=new MeshRouter();
               router->SetID(routerID);
               router->SetStrategy(strategy);
               routingEngine->AddRouter(router);
               s.append("\tRouting Strategy nav_mesh  router added\n");
               break;
          }
          case ROUTING_DUMMY: {
               Router* router=new DummyRouter();
               router->SetID(routerID);
               router->SetStrategy(strategy);
               routingEngine->AddRouter(router);
               s.append("\tRouting Strategy dummy router added\n");
               break;
          }
          case ROUTING_SAFEST: {
               Router * router=new SafestPathRouter();
               router->SetID(routerID);
               router->SetStrategy(strategy);
               routingEngine->AddRouter(router);
               s.append("\tRouting Strategy cognitive map router added\n");
               break;
          }
          case ROUTING_COGNITIVEMAP: {
               Router* router=new CognitiveMapRouter();
               router->SetID(routerID);
               router->SetStrategy(strategy);
               routingEngine->AddRouter(router);
               s.append("\tRouting Strategy dummy router added\n");
               break;
          }
          case ROUTING_UNDEFINED:
          default:
               cout<<"router not available"<<endl;
               exit(EXIT_FAILURE);
               break;
          }
     }
     s.append("\n");

     // IMPORTANT: do not change the order in the following..
     _building = new Building();
     _building->SetRoutingEngine(routingEngine);
     _building->SetProjectFilename(args->GetProjectFile());
     _building->SetProjectRootDir(args->GetProjectRootDir());

     _building->LoadGeometry();
     _building->LoadRoutingInfo(args->GetProjectFile());
     //_building->AddSurroundingRoom();
     _building->InitGeometry(); // create the polygons
     _building->LoadTrafficInfo();


     _nPeds=_distribution->Distribute(_building);

     //using linkedcells
     if (args->GetLinkedCells()) {
          s.append("\tusing Linked-Cells for spatial queries\n");
          _building->InitGrid(args->GetLinkedCellSize());
     } else {
          _building->InitGrid(-1);
     }

     // initialize the routing engine before doing any other things
     routingEngine->Init(_building);

     //perform customs initialisation, like computing the phi for the gcfm
     //this should be called after the routing engine has been initialised
     // because a direction is needed for this initialisation.
     _operationalModel->Init(_building);

     //other initializations
     const vector< Pedestrian* >& allPeds = _building->GetAllPedestrians();
     for(Pedestrian *ped: allPeds)
        {
         ped->Setdt(_deltaT);
        }
     //pBuilding->WriteToErrorLog();

     //get the seed
     _seed=args->GetSeed();

     // perform a general check to the .
     _building->SanityCheck();
     //size of the cells/GCFM/Gompertz
     if(args->GetDistEffMaxPed()>args->GetLinkedCellSize()){
          Log->Write("ERROR: the linked-cell size [%f] should be bigger than the force range [%f]",
                  args->GetLinkedCellSize(),args->GetDistEffMaxPed());
          exit(EXIT_FAILURE);
     }

     //read the events
     _em = new EventManager(_building);
     _em->SetProjectFilename(args->GetProjectFile());
     _em->SetProjectRootDir(args->GetProjectRootDir());
     _em->readEventsXml();
     _em->listEvents();

     //which hpc-architecture?
     _hpc = args->GetHPCFlag();
     //if programming model = ocl create buffers and make the setup
     //if(_hpc==1){
     //((GPU_ocl_GCFMModel*) _model)->CreateBuffer(_building->GetNumberOfPedestrians());
     //((GPU_ocl_GCFMModel*) _model)->initCL(_building->GetNumberOfPedestrians());
     //}
     //_building->SaveGeometry("test.sav.xml");
}


int Simulation::RunSimulation()
{
     int frameNr = 1; // Frame Number
     int writeInterval = (int) ((1. / _fps) / _deltaT + 0.5);
     writeInterval = (writeInterval <= 0) ? 1 : writeInterval; // mustn't be <= 0
     double t=0.0;

     // writing the header
     _iod->WriteHeader(_nPeds, _fps, _building,_seed);
     _iod->WriteGeometry(_building);
     _iod->WriteFrame(0,_building);

     //first initialisation needed by the linked-cells
     UpdateRoutesAndLocations();

     //needed to control the execution time PART 1
     //in the case you want to run in no faster than realtime
     //time_t starttime, endtime;
     //time(&starttime);

     // main program loop
     for (t = 0; t < _tmax && _nPeds > 0; ++frameNr)
     {
          t = 0 + (frameNr - 1) * _deltaT;

          // update the positions
          _operationalModel->ComputeNextTimeStep(t,_deltaT,_building);

          // update the routes and locations
          //Update();
          UpdateRoutesAndLocations();

          //update the events
          _em->Update_Events(t,_deltaT);

          //other updates
          //someone might have leave the building
          _nPeds=_building->GetAllPedestrians().size();

          //update the linked cells
          _building->UpdateGrid();

          // update the global time
           Pedestrian::SetGlobalTime(t);

          // write the trajectories
          if (frameNr % writeInterval == 0) {
               _iod->WriteFrame(frameNr / writeInterval, _building);
          }

          // needed to control the execution time PART 2
          // time(&endtime);
          // double timeToWait=t-difftime(endtime, starttime);
          // clock_t goal = timeToWait*1000 + clock();
          // while (goal > clock());

     }
     // writing the footer
     _iod->WriteFooter();

     //if(_hpc==1)
       //  ((GPU_GCFMModel*) _model)->DeleteBuffers();

     //temporary work around since the total number of frame is only available at the end of the simulation.
     if(_argsParser->GetFileFormat()==FORMAT_XML_BIN) {

          delete _iod;   _iod=NULL;

          // char tmp[CLENGTH];
          // int f= frameNr / writeInterval ;
          // sprintf(tmp,"<frameCount>%07d</frameCount>",f);
          // string frameCount (tmp);

          char replace[CLENGTH];
          // open the file and replace the 8th line
          sprintf(replace,"sed -i '9s/.*/ %d /' %s", frameNr/ writeInterval, _argsParser->GetTrajectoriesFile().c_str());
          int result=system(replace);
          Log->Write("INFO:\t Updating the framenumber exits with code [%d]",result);
     }

     //return the evacuation time
     return (int) t;
}


void Simulation::UpdateRoutesAndLocations()
{
     //pedestrians to be deleted
     //you should better create this in the constructor and allocate it once.
     vector<Pedestrian*> pedsToRemove;
     pedsToRemove.reserve(100); //just reserve some space

     // collect all pedestrians in the simulation.
     const vector< Pedestrian* >& allPeds = _building->GetAllPedestrians();
     const map<int, Goal*>& goals=_building->GetAllGoals();

     unsigned int nSize = allPeds.size();
     int nThreads = omp_get_max_threads();
     int partSize = nSize / nThreads;

#pragma omp parallel  default(shared) num_threads(nThreads)
     {
          const int threadID = omp_get_thread_num();
          int start = threadID * partSize;
          int end = (threadID + 1) * partSize - 1;
          if ((threadID == nThreads - 1))
               end = nSize - 1;

          for (int p = start; p <= end; ++p)
          {
               Pedestrian* ped = allPeds[p];
               Room* room = _building->GetRoom(ped->GetRoomID());
               SubRoom* sub = room->GetSubRoom(ped->GetSubRoomID());

               //set the new room if needed
               if ((ped->GetFinalDestination() == FINAL_DEST_OUT)
                         && (room->GetCaption() == "outside"))
               {
#pragma omp critical
                    pedsToRemove.push_back(ped);
               }
               else if ((ped->GetFinalDestination() != FINAL_DEST_OUT)
                         && (goals.at(ped->GetFinalDestination())->Contains(ped->GetPos())))
               {
#pragma omp critical
                    pedsToRemove.push_back(ped);
               }

               // reposition in the case the pedestrians "accidently left the room" not via the intended exit.
               // That may happen if the forces are too high for instance
               // the ped is removed from the simulation, if it could not be reassigned
               else if (!sub->IsInSubRoom(ped))
               {
                    bool assigned = false;
                    const std::vector<Room*>& allRooms=_building->GetAllRooms();
                    for (Room* room : allRooms)
                    {
                         const vector<SubRoom*>& allSubs=room->GetAllSubRooms();
                         for(SubRoom* sub : allSubs)
                         {
                              SubRoom* old_sub= allRooms[ped->GetRoomID()]->GetSubRoom(ped->GetSubRoomID());
                              if ((sub->IsInSubRoom(ped->GetPos())) && (sub->IsDirectlyConnectedWith(old_sub))) {
                                   ped->SetRoomID(room->GetID(), room->GetCaption());
                                   ped->SetSubRoomID(sub->GetSubRoomID());
                                   ped->ClearMentalMap(); // reset the destination
                                   //ped->FindRoute();
                                   assigned = true;
                                   break;
                              }
                         }
                         if (assigned == true)
                              break; // stop the loop
                    }

                    if (assigned == false)
                    {
#pragma omp critical
                         pedsToRemove.push_back(ped);
                    }
               }

               //finally actualize the route
               if (ped->FindRoute() == -1) {
                    //a destination could not be found for that pedestrian
                    Log->Write("\tINFO: \tCould not found a route for pedestrian %d",ped->GetID());
#pragma omp critical
                    pedsToRemove.push_back(ped);
               }
          }
     }


     // remove the pedestrians that have left the building
     for (unsigned int p = 0; p < pedsToRemove.size(); p++)
     {
          _building->DeletePedestrian(pedsToRemove[p]);
     }


     //    temporary fix for the safest path router
     //    if (dynamic_cast<SafestPathRouter*>(_routingEngine->GetRouter(1)))
     //    {
     //         SafestPathRouter* spr = dynamic_cast<SafestPathRouter*>(_routingEngine->GetRouter(1));
     //         spr->ComputeAndUpdateDestinations(_allPedestians);
     //    }
}

void Simulation::PrintStatistics()
{
     Log->Write("\nEXIT USAGE:");
     const map<int, Transition*>& transitions = _building->GetAllTransitions();
     map<int, Transition*>::const_iterator itr;
     for(itr = transitions.begin(); itr != transitions.end(); ++itr) {
          Transition* goal =  itr->second;
          if(goal->IsExit()) {
               Log->Write("Exit ID [%d] used by [%d] pedestrians. Last passing time [%0.2f] s",goal->GetID(),goal->GetDoorUsage(),goal->GetLastPassingTime());
          }
     }
}
