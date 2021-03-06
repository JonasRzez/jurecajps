/**
 * \file        Building.h
 * \date        Oct 1, 2010
 * \version     v0.7
 * \copyright   <2009-2015> Forschungszentrum Jülich GmbH. All rights reserved.
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
 *
 *
 **/
#pragma once

#include "Goal.h"
#include "Hline.h"
#include "NavLine.h"
#include "Obstacle.h"
#include "Room.h"
#include "TrainGeometryInterface.h"
#include "Transition.h"
#include "Wall.h"
#include "general/Configuration.h"
#include "general/Filesystem.h"
#include "neighborhood/NeighborhoodSearch.h"

#include <optional>

using PointWall = std::pair<Point, Wall>;

class RoutingEngine;

class Pedestrian;

class Transition;

class PedDistributor;

class Configuration;

class WaitingArea;

class Building
{
private:
    Configuration * _configuration = nullptr;
    std::shared_ptr<RoutingEngine> _routingEngine;
    std::string _caption;
    std::string _geometryFilename;
    NeighborhoodSearch _neighborhoodSearch;
    std::vector<Pedestrian *> _allPedestrians;
    std::map<int, std::shared_ptr<Room>> _rooms;
    std::map<int, Crossing *> _crossings;
    std::map<int, Transition *> _transitions;
    std::map<int, Hline *> _hLines;
    std::map<int, Goal *> _goals;
    /// pedestrians pathway
    bool _savePathway;
    std::ofstream _pathWayStream;
    std::map<int, TrainType> _trains;

    std::map<int, Track> _tracks;
    std::map<int, Point> _trackStarts;

    /**
     * Map of walls added temporarily for a specific train
     */
    std::map<int, std::vector<Wall>> _trainWallsAdded;

    /**
     * Map of walls removed temporarily for a specific train
     */
    std::map<int, std::vector<Wall>> _trainWallsRemoved;

    /**
     * Map of doors added temporarily for a specific train
     */
    std::map<int, std::vector<Transition>> _trainDoorsAdded;

public:
    /// constructor
    Building();

    Building(Configuration * config, PedDistributor & pedDistributor);

    /// destructor
    virtual ~Building();

    Configuration * GetConfig() const;

    void SetCaption(const std::string & s);

    /// delete the ped from the ped vector
    void DeletePedestrian(Pedestrian *& ped);

    /// delete the ped from the simulation
    void AddPedestrian(Pedestrian * ped);

    void GetPedestrians(int room, int subroom, std::vector<Pedestrian *> & peds) const;

    std::string GetCaption() const;

    RoutingEngine * GetRoutingEngine() const;
    const std::map<int, std::shared_ptr<Room>> & GetAllRooms() const;
    const std::vector<Pedestrian *> & GetAllPedestrians() const;

    Pedestrian * GetPedestrian(int pedID) const;

    int GetNumberOfRooms() const;

    int GetNumberOfGoals() const;

    Room * GetRoom(int index) const;

    Room * GetRoom(std::string caption) const;

    Transition * GetTransition(std::string caption) const;

    Transition * GetTransition(int id) const;

    /**
      * Returns Crossing with a specified ID
      *
      * @param ID of Crossing: int
      * @return Pointer of Crossing
      */
    Crossing * GetCrossing(int ID) const;

    /**
      * Not implemented
      */
    Hline * GetHline(int id);

    /**
      * return the subroom with the corresponding unique identifier
      * @param uid ,the unique identifier
      * @return NULL if no exists with that identifier.
      */
    SubRoom * GetSubRoomByUID(int uid) const;

    /**
      * @return true if the two segments are visible from each other.
      * Alls walls and transitions and crossings are used in this check.
      * The use of hlines is optional, because they are not real, can can be considered transparent
      */
    //bool IsVisible(Line* l1, Line* l2, bool considerHlines=false);

    /**
      * @return true if the two points are visible from each other.
      * Alls walls and transitions and crossings are used in this check.
      * The use of hlines is optional, because they are not real, can be considered transparent
      */
    bool IsVisible(
        const Point & p1,
        const Point & p2,
        const std::vector<SubRoom *> & subrooms,
        bool considerHlines = false);

    /**
      * @return a crossing or a transition matching the given caption.
      * Return NULL if none is found
      */
    Crossing * GetTransOrCrossByName(std::string caption) const;

    /**
      * @return a crossing or a transition or a hline matching the given uid.
      * Return NULL if none is found
      */
    Hline * GetTransOrCrossByUID(int uid) const;

    /**
      * @return the transition matching the uid
      */
    Transition * GetTransitionByUID(int uid) const;

    Crossing * GetCrossingByUID(int uid) const;

    //TOD0: rename later to GetGoal
    Goal * GetFinalGoal(int id) const;

    const NeighborhoodSearch & GetNeighborhoodSearch() const;

    // convenience methods
    bool InitGeometry();


    void InitGrid();

    void InitSavePedPathway(const std::string & filename);

    void AddRoom(Room * room);

    void UpdateGrid();

    void
    AddSurroundingRoom(); // add a final room (outside or world), that encompasses the complete geometry

    const std::map<int, Crossing *> & GetAllCrossings() const;

    const std::map<int, Transition *> & GetAllTransitions() const;

    const std::map<int, Hline *> & GetAllHlines() const;

    const std::map<int, Goal *> & GetAllGoals() const;

    // --------------- Trains interface
    const std::map<int, Track> & GetTracks() const;

    const std::vector<Wall>
    GetTrackWalls(Point TrackStart, Point TrackEnd, int & room_id, int & subroom_id) const;

    void AddTrainWallAdded(int trainID, Wall trainAddedWall);
    void ClearTrainWallsAdded(int trainID);
    std::optional<std::vector<Wall>> GetTrainWallsAdded(int trainID);

    void AddTrainWallRemoved(int trainID, Wall trainRemovedWall);
    void ClearTrainWallsRemoved(int trainID);
    std::optional<std::vector<Wall>> GetTrainWallsRemoved(int trainID);

    void AddTrainDoorAdded(int trainID, Transition trainAddedDoor);
    void ClearTrainDoorsAdded(int trainID);
    std::optional<std::vector<Transition>> GetTrainDoorsAdded(int trainID);

    // ------------------------------------
    bool AddCrossing(Crossing * line);

    bool RemoveTransition(Transition * line);

    bool AddTransition(Transition * line);

    bool AddHline(Hline * line);

    bool AddGoal(Goal * goal);

    const fs::path & GetProjectRootDir() const;

    const fs::path & GetProjectFilename() const;

    const fs::path & GetGeometryFilename() const;

    /**
      * Write the geometry to the given file.
      * That will be useful in the geometry editor.
      * @param filename the relative location of the file
      * @return true if everything went fine.
      */
    bool SaveGeometry(const fs::path & filename) const;

    /**
      * Check the scenario for possible errors and
      * output user specific informations.
      */
    bool SanityCheck();

    /**
      * Triangulate the geometry
      */
    bool Triangulate();

    /**
      * @return Vector with the vertices of the geometry's outer boundary rect
      */
    std::vector<Point> GetBoundaryVertices() const;

    /**
     * Adds a train to the building
     * @param trainID ID of the added train
     * @param type type of the train
     */
    void AddTrain(int trainID, TrainType type);

    /**
     * Get the type of train with ID \p trainID
     * @param trainID ID of the train
     * @return Type of train with ID \p trainID
     */
    TrainType GetTrain(int trainID);

    /**
     * Get the train types as map
     * @return train types of the building with trainID as key
     */
    std::map<int, TrainType> GetTrains() const;

    /**
     * Get the train types as vector, each type only once
     * @return vector of train types in \a _trains
     */
    std::vector<TrainType> GetTrainTypes();

    void AddTrackWall(int trackID, int roomID, int subRoomID, Wall trackWall);

    std::optional<Track> GetTrack(int track) const;

    void AddTrackStart(int trackID, Point trackStart);

    std::optional<Point> GetTrackStart(int trackID) const;

private:
    bool InitInsideGoals();
    void InitPlatforms();
    void StringExplode(std::string str, std::string separator, std::vector<std::string> * results);
};
