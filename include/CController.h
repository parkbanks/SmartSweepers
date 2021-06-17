#ifndef CCONTROLLER_H
#define CCONTROLLER_H

//------------------------------------------------------------------------
//
//	Name: CController.h
//
//  Author: Mat Buckland 2002
//
//  Desc: Controller class for the 'Smart Sweeper' example
//
//------------------------------------------------------------------------
#include <vector>
#include <sstream>
#include <string>
#include <fstream>
#include <windows.h>
#include <iostream>
#include <math.h>

#include "CMinesweeper.h"
#include "CGenAlg.h"
#include "utils.h"
#include "C2DMatrix.h"
#include "SVector2D.h"
#include "CParams.h"

using namespace std;



class CController
{

private:

	//storage for the population of genomes
	vector<SGenome>	     m_vecThePopulation;
	//vector<SGenome>	     m_vecThePopulation1;

	//and the minesweepers
  vector<CMinesweeper> m_vecSweepers;

  vector<SVector2D>	   botVector;

	//and the mines
	vector<SVector2D>	   m_vecMines;

	//pointer to the GA
	CGenAlg*		         m_pGA;
	//CGenAlg*		         m_pGA1;

	int					         m_NumSweepers;

	int					         m_NumMines;

	int					         m_NumWeightsInNN;

	float* aveSpeed;
	float* aveRotation;
	float* radians;
	float* aveMineDist;
	float* aveBotDist;
	float* xrad;
	float* yrad;

	//vertex buffer for the sweeper shape's vertices
	vector<SPoint>		   m_SweeperVB;

	//vertex buffer for the mine shape's vertices
	vector<SPoint>		   m_MineVB;

	//stores the average fitness per generation for use
	//in graphing.
	vector<double>		   m_vecAvFitness;

	//stores the best fitness per generation
	vector<double>		   m_vecBestFitness;



	//pens we use for the stats
	HPEN				m_RedPen;
	HPEN                m_OrangePen;
	HPEN				m_GreenPen;
	HPEN				m_BluePen;
	HPEN                m_IndigoPen;
    HPEN                m_BlackPen;
	HPEN				m_OldPen;

	//handle to the application window
	HWND				m_hwndMain;

	//toggles the speed at which the simulation runs
	bool				m_bFastRender;

	//cycles per generation
	int					m_iTicks;

	//generation counter
	int					m_iGenerations;

  //window dimensions
  int         cxClient, cyClient;

  //this function plots a graph of the average and best fitnesses
  //over the course of a run
  void   PlotStats(HDC surface);

public:

	CController(HWND hwndMain);

	~CController();

	void		Render(HDC surface);

	void		WorldTransform(vector<SPoint> &VBuffer,
                         SVector2D      vPos);

	bool		Update();


	//accessor methods
	bool		FastRender()const	  {return m_bFastRender;}
	void		FastRender(bool arg){m_bFastRender = arg;}
	void		FastRenderToggle()  {m_bFastRender = !m_bFastRender;}
	vector<CMinesweeper> outSweepers()const{return m_vecSweepers;}
	std::ofstream myfile;

};


#endif

