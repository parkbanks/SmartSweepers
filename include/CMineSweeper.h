#ifndef CMINESWEEPER_H
#define CMINESWEEPER_H

//------------------------------------------------------------------------
//
//	Name: CMineSweeper.h
//
//  Author: Mat Buckland 2002
//
//  Desc: Class to create a minesweeper object
//
//------------------------------------------------------------------------
#include <vector>
#include <math.h>

#include "CNeuralNet.h"
#include "utils.h"
#include "C2DMatrix.h"
#include "SVector2D.h"
#include "CParams.h"

using namespace std;


class CMinesweeper
{

private:

    //the minesweeper's neural net
    CNeuralNet m_ItsBrain;

	//its position in the world
	SVector2D m_vPosition;

	//direction sweeper is facing
	SVector2D		m_vLookAt;

	//its rotation (surprise surprise)
	double			m_dRotation;

	double			m_dSpeed;//attach to fitness score

	//to store output from the ANN
	double			m_lTrack, m_rTrack;
	double          m_blTrack, m_brTrack;

	//the sweeper's fitness score
	double			m_dFitness;

	//the scale of the sweeper when drawn
	double			m_dScale;

    //index position of closest mine
    int         m_iClosestMine;

    int m_iClosestBot;

    int m_Energy;

    //double stomachType;
    int stomachType;



public:


	CMinesweeper();

	//updates the ANN with information from the sweepers environment
	bool			Update(vector<SVector2D> &mines, vector<SVector2D> &bots, vector<CMinesweeper> &bots2);

	//used to transform the sweepers vertices prior to rendering
  void			WorldTransform(vector<SPoint> &sweeper);

	//returns a vector to the closest mine
  SVector2D	GetClosestMine(vector<SVector2D> &objects);

    //returns a vector to the closest bot
    SVector2D GetClosestBot(vector<SVector2D> &obj);

    //vector<SVector2D> vecMine;

    //vector<SVector2D> vecBot;

    float vecMinex;
    float vecMiney;
    float vecBotx;
    float vecBoty;

    //checks to see if the minesweeper has 'collected' a mine
    int       CheckForMine(vector<SVector2D> &mines, double size);

    //checks to see if the minesweeper has collided with a bot
    int CheckForBot(vector<SVector2D> &bots, double size);

    int CheckEnergy(vector<CMinesweeper> &bots2);

    double CheckStomach(vector<CMinesweeper> &bots2);

	void			Reset();


	//-------------------accessor functions
	SVector2D	Position()const{return m_vPosition;}

	SVector2D	LookAt()const{return m_vLookAt;}

	double getStomach()const{return stomachType;}

	int getEnergy()const{return m_Energy;}

	double		Fitness()const{return m_dFitness;}
	double		Speed()const{return m_dSpeed;}
	double		Rotation()const{return m_dRotation;}

	void			IncrementFitness(){++m_dFitness;}

	void			DecrementFitness(){--m_dFitness;}

	void			IncrementEnergy(){++m_Energy;}

	void			DecrementEnergy(){--m_Energy;}

    void      PutWeights(vector<double> &w){m_ItsBrain.PutWeights(w);}

    int       GetNumberOfWeights()const{return m_ItsBrain.GetNumberOfWeights();}
};


#endif


