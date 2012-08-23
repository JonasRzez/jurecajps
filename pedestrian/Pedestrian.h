/**
 * File:   Pedestrian.h
 *
 *Created on 30. September 2010, 15:59
 *
 * @section LICENSE
 * This file is part of JuPedSim.
 *
 * JuPedSim is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * JuPedSim is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with JuPedSim. If not, see <http://www.gnu.org/licenses/>.
 *
 * @section DESCRIPTION
 *
 *
 *
 */

#ifndef _PEDESTRIAN_H
#define	_PEDESTRIAN_H

#include <vector>
#include <map>

#include "../pedestrian/Ellipse.h"
#include "../geometry/Point.h"
#include "../general/Macros.h"

class Building;

class Pedestrian {
private:
	int pPedIndex; // Index des Fußgängers 1,..., N
	double pHeight;
	double pAge;
	string pGender;

	int pRoomID;
	int pSubRoomID;
	string pRoomCaption;
	double pMass; // Masse: 1
	double pTau; // Reaktinszeit: 0.5
	double pDt; // step size
	Ellipse pEllipse;
	int pExitIndex; // aktueller Ausgang, Index in routing->GetGoal(index)
	Line* pExitLine; // aktuelle Ausgangslinie
	std::map<int, int>pMentalMap; // map the actual room to a destination
	std::vector<int> pDestHistory;
	//int pMentalMapArray[15][130];
	Point pV0; //vector V0
	Point pLastPosition;
	vector<int> pTrip;
	int pGroup;

	//routing parameters
	double pReroutingThreshold; // new orientation after 10 seconds
	double pTimeBeforeRerouting; // a new orientation starts after this time
	bool  pReroutingEnabled;
	double pTimeInJam; // actual time im Jam
	int pDesiredFinalDestination;
	double pPatienceTime; // time after which the ped feels to be in jam

	// necessary for smooth turning at sharp bend
	bool pNewOrientationFlag;
	int pNewOrientationDelay; //2 seconds, in steps
	int pUpdateRate;
	double pTurninAngle;
	bool tmpFirstOrientation; // possibility to get rid of this variable


public:
	// Konstruktoren
	Pedestrian();
	Pedestrian(const Pedestrian& orig);
	virtual ~Pedestrian();

	// Setter-Funktionen
	void SetPedIndex(int i);
	//TODO: use setRoom(Room*) and setSubRoom(SubRoom*)
	void SetRoomID(int i, string roomCaption);
	void SetSubRoomID(int i);
	void SetMass(double m);
	void SetTau(double tau);
	void SetEllipse(const Ellipse& e);
	void SetExitIndex(int i);
	void SetExitLine(Line* l);
	void Setdt(double dt);

	// Eigenschaften der Ellipse
	void SetPos(const Point& pos); // setzt x und y-Koordinaten
	void SetV(const Point& v); // setzt x und y-Koordinaten der Geschwindigkeit
	void SetV0Norm(double v0);
	void SetSmoothTurning(bool smt); // activate the smooth turning with a delay of 2 sec
	void SetPhiPed();
	void SetFinalDestination(int UID);
	void SetTrip(vector<int> trip);

	// Getter-Funktionen
	const vector<int> GetTrip() const;
	int GetPedIndex() const;
	int GetRoomID() const;
	int GetSubRoomID() const;
	double GetMass() const;
	double GetTau() const;
	const Ellipse& GetEllipse() const;
	int GetExitIndex() const;
	Line* GetExitLine() const;
	// Eigenschaften der Ellipse
	const Point& GetPos() const;
	const Point& GetV() const;
	const Point& GetV0() const;
	const Point& GetV0(const Point& target);
	double GetV0Norm() const;
	double GetLargerAxis() const;
	double GetSmallerAxis() const;
	double GetTimeInJam()const;
	int GetFinalDestination() const;
	void ClearMentalMap(); // erase the peds memory

	int GetUniqueRoomID() const;
	int GetNextDestination();
	int GetLastDestination();
	double GetDistanceToNextTarget() const;
	double GetDisTanceToPreviousTarget() const;

	void RecordActualPosition();
	double GetDistanceSinceLastRecord();

	///write the pedestrian path (room and exit taken ) to file
	void WritePath(ofstream& file, Building* building=NULL);

	///write the pedestrian path (room and exit taken ) to file
	/// in the format room1:exit1>room2:exit2
	string GetPath();

	//debug
	void Dump(int ID, int pa = 0); // dump pedestrians parameter, 0 for all parameters

	/**
	 * observe the reference pedestrians and collect some data, e.g distance to exit
	 * and average/maximal velocities
	 *
	 * @param maxObservationTime in sec.
	 * @return false, if the observation time is over and the observation data can be retrieved
	 */
	bool Observe(double maxObservationTime=-1);

	/**
	 * @return true, if reference pedestrian have been selected and the observation process has started
	 */
	bool IsObserving();

	/**
	 * return the observation data in an array
	 *
	 * @param exitID, the concerned exit
	 * @param data, a float array to store the data
	 */
	void GetObservationData(int exitID, float* data);

	/**
	 * @return true if the time spent in jam exceed the given threshold
	 */
	bool IsFeelingLikeInJam();

	void ResetTimeInJam();
	void UpdateTimeInJam();
	void UpdateJamData();
	void UpdateReroutingTime();
	void RerouteIn(double time);
	bool IsReadyForRerouting();

	/**
	 * clear the parameter related to the re routing
	 */
	void ResetRerouting();


	//void SetMaxOberservationTime(double time);
	double GetAge() const;
	void SetAge(double age);
	string GetGender() const;
	void SetGender(string gender);
	double GetHeight() const;
	void SetHeight(double height);
	int GetGroup() const;
	void SetGroup(int group);
};

#endif	/* _PEDESTRIAN_H */

