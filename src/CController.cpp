#include "CController.h"


//these hold the geometry of the sweepers and the mines, all as x,y
const int	 NumSweeperVerts = 4;//glitches if vertices changed to 13
const SPoint sweeper[NumSweeperVerts] = {SPoint(0, -0.5), //bottom Center
                                         SPoint(-1, -0.5), //bottom right corner
                                         SPoint(0, 1.75),//point
                                         SPoint(1, -0.5),//bottom left


};
const int NumMineVerts = 4;
const SPoint mine[NumMineVerts] = {SPoint(-1, -1),
                                   SPoint(-1, 1),
                                   SPoint(1, 1),
                                   SPoint(1, -1)};



//---------------------------------------constructor---------------------
//
//	initilaize the sweepers, their brains and the GA factory
//
//-----------------------------------------------------------------------
CController::CController(HWND hwndMain): m_NumSweepers(CParams::iNumSweepers),
										                     m_pGA(NULL),
										                     /*m_pGA1(NULL),*/
										                     m_bFastRender(false),
										                     m_iTicks(0),
										                     m_NumMines(CParams::iNumMines),
										                     m_hwndMain(hwndMain),
										                     m_iGenerations(0),
                                         cxClient(CParams::WindowWidth),
                                         cyClient(CParams::WindowHeight)
{
    aveSpeed = new float[m_NumSweepers];
    aveRotation = new float[m_NumSweepers];
    xrad = new float[m_NumSweepers];
    yrad = new float[m_NumSweepers];
    //radians = new float[m_NumSweepers];
    aveMineDist = new float[m_NumSweepers];
    aveBotDist = new float[m_NumSweepers];
    myfile.open("mydata.txt");
    if (myfile.is_open())
    {
        myfile << "Simulation, Population, Trial, Generation, Bot#, Stomach, Energy, Fitness, AveSpeed, AveRotation, AveOrientation, AveMineDist, AveBotDist, \n";
    }
	//let's create the mine sweepers
	int car = 0;
	int her = 0;
	for (int i=0; i<m_NumSweepers; ++i)
	{
		m_vecSweepers.push_back(CMinesweeper());
		botVector.push_back(m_vecSweepers[i].Position());
		if(m_vecSweepers[i].getStomach() == 0)
        {
            her++;
        }
        else if(m_vecSweepers[i].getStomach() == 1)
        {
            car++;
        }
	}

	//get the total number of weights used in the sweepers
	//NN so we can initialise the GA
	m_NumWeightsInNN = m_vecSweepers[0].GetNumberOfWeights();

	//initialize the Genetic Algorithm class
	//herbivores
	m_pGA = new CGenAlg(her,
                      CParams::dMutationRate,
	                    CParams::dCrossoverRate,
	                    m_NumWeightsInNN);

    //carnivores
    /*m_pGA1 = new CGenAlg(car,
                      CParams::dMutationRate,
	                    CParams::dCrossoverRate,
	                    m_NumWeightsInNN);*/

	//Get the weights from the GA and insert into the sweepers brains
	m_vecThePopulation = m_pGA->GetChromos();
	//m_vecThePopulation1 = m_pGA1->GetChromos();

    int i;
    her = 0;
    car = 0;
	for (i=0; i<m_NumSweepers; i++)
    {
        if(m_vecSweepers[i].getStomach() == 0)
        {
            m_vecSweepers[i].PutWeights(m_vecThePopulation[her].vecWeights);
            her++;
        }
        /*
        else if(m_vecSweepers[i].getStomach() == 1)
        {
            m_vecSweepers[i].PutWeights(m_vecThePopulation1[car].vecWeights);
            car++;
        }
        */
    }




	//initialize mines in random positions within the application window
	for (i=0; i<m_NumMines; ++i)
	{
		m_vecMines.push_back(SVector2D(RandFloat() * cxClient,
                                   RandFloat() * cyClient));
	}

	//create a pen for the graph drawing
    m_RedPen = CreatePen(PS_SOLID, 1, RGB(255, 0, 0));
    m_OrangePen = CreatePen(PS_SOLID, 1, RGB(255, 165, 0));
    m_GreenPen = CreatePen(PS_SOLID, 1, RGB(0, 128, 0));
    m_BluePen = CreatePen(PS_SOLID, 1, RGB(0, 0, 255));
    m_IndigoPen = CreatePen(PS_SOLID, 1, RGB(75, 0, 130));
    m_BlackPen = CreatePen(PS_SOLID, 1, RGB(0, 0, 0));
	m_OldPen	= NULL;

	//fill the vertex buffers
	for (i=0; i<NumSweeperVerts; ++i)
	{
		m_SweeperVB.push_back(sweeper[i]);
	}

	for (i=0; i<NumMineVerts; ++i)
	{
		m_MineVB.push_back(mine[i]);
	}

}


//--------------------------------------destructor-------------------------------------
//
//--------------------------------------------------------------------------------------
CController::~CController()
{
	if(m_pGA)
  {
    delete		m_pGA;
  }
  /*
  if(m_pGA1)
  {
    delete		m_pGA1;
  }*/

    delete[] aveSpeed;
    delete[] aveRotation;
    //delete[] radians;
    delete[] xrad;
    delete[] yrad;
    delete[] aveMineDist;
    delete[] aveBotDist;
	DeleteObject(m_BluePen);
	DeleteObject(m_RedPen);
	DeleteObject(m_GreenPen);
	DeleteObject(m_IndigoPen);
	DeleteObject(m_BlackPen);
	DeleteObject(m_OrangePen);
	DeleteObject(m_OldPen);

}


//---------------------WorldTransform--------------------------------
//
//	sets up the translation matrices for the mines and applies the
//	world transform to each vertex in the vertex buffer passed to this
//	method.
//-------------------------------------------------------------------
void CController::WorldTransform(vector<SPoint> &VBuffer, SVector2D vPos)
{
	//create the world transformation matrix
	C2DMatrix matTransform;

	//scale
	matTransform.Scale(CParams::dMineScale, CParams::dMineScale);

	//translate
	matTransform.Translate(vPos.x, vPos.y);

	//transform the ships vertices
	matTransform.TransformSPoints(VBuffer);
}


//-------------------------------------Update-----------------------------
//
//	This is the main workhorse. The entire simulation is controlled from here.
//
//	The comments should explain what is going on adequately.
//-------------------------------------------------------------------------
bool CController::Update()
{

	//run the sweepers through CParams::iNumTicks amount of cycles. During
  //this loop each sweepers NN is constantly updated with the appropriate
  //information from its surroundings. The output from the NN is obtained
  //and the sweeper is moved. If it encounters a mine its fitness is
  //updated appropriately,
    int car = 0;
	int her = 0;
	if (m_iTicks++ < CParams::iNumTicks)
    {
        for (int i=0; i<m_NumSweepers; ++i)
        {
            aveSpeed[i] += m_vecSweepers[i].Speed();
            aveRotation[i] += m_vecSweepers[i].Rotation();

			//update the NN and position
			if (!m_vecSweepers[i].Update(m_vecMines, botVector, m_vecSweepers))
            {
				//error in processing the neural net
				MessageBox(m_hwndMain, "Wrong amount of NN inputs!", "Error", MB_OK);

				return false;
			}

			//see if it's found a mine
            int GrabHit = m_vecSweepers[i].CheckForMine(m_vecMines,
                                                  CParams::dMineScale);
            if(m_vecSweepers[i].getStomach() == 0)
            {
                if (GrabHit >= 0)//0 as if no hit return -1
                {
                //we have discovered a mine so increase fitness
                m_vecSweepers[i].IncrementFitness();
                m_vecSweepers[i].IncrementEnergy();

                //mine found so replace the mine with another at a random
                //position
                m_vecMines[GrabHit] = SVector2D(RandFloat() * cxClient,
                                        RandFloat() * cyClient);
                }
            }
            if (myfile.is_open())
            {
                //cout << m_vecSweepers[i].LookAt().y << ", " << m_vecSweepers[i].LookAt().x << endl;
                xrad[i] += m_vecSweepers[i].LookAt().x;
                yrad[i] += m_vecSweepers[i].LookAt().y;
                //radians[i] += atan2(m_vecSweepers[i].LookAt().y, m_vecSweepers[i].LookAt().x) * 180 / 3.14159265;
                aveMineDist[i] += sqrt((m_vecSweepers[i].vecMinex*m_vecSweepers[i].vecMinex) +
                                      (m_vecSweepers[i].vecMiney*m_vecSweepers[i].vecMiney));
            }

            //see if it has found a bot
            int GrabHit1 = m_vecSweepers[i].CheckForBot(botVector, CParams::iSweeperScale);

            if (GrabHit1 >= 0)//change to 0 // right now it is comparing its stomach against itself
            {
                if(m_vecSweepers[i].getStomach() > m_vecSweepers[i].CheckStomach(m_vecSweepers))
                {
                    int tempEnergy = m_vecSweepers[i].CheckEnergy(m_vecSweepers);
                    if(tempEnergy > 0)
                    {
                        //we have discovered a bot so increase fitness
                        m_vecSweepers[i].IncrementFitness();
                        m_vecSweepers[i].IncrementEnergy();
                    }
                }

                else if(m_vecSweepers[i].getStomach() < m_vecSweepers[i].CheckStomach(m_vecSweepers))
                {
                    if(m_vecSweepers[i].getEnergy() > 0 && m_vecSweepers[i].Fitness() > 0)
                    {
                        m_vecSweepers[i].DecrementFitness();
                        m_vecSweepers[i].DecrementEnergy();
                    }
                }

                else if(m_vecSweepers[i].getStomach() == m_vecSweepers[i].CheckStomach(m_vecSweepers)){}
            }
            if (myfile.is_open())
            {
                aveBotDist[i] += sqrt((m_vecSweepers[i].vecBotx*m_vecSweepers[i].vecBotx) +
                                      (m_vecSweepers[i].vecBoty*m_vecSweepers[i].vecBoty));
            }
            //update the chromos fitness score
            if(m_vecSweepers[i].getStomach() == 0)
            {
                m_vecThePopulation[her].dFitness = m_vecSweepers[i].Fitness();
                her++;
            }
            /*
            else if(m_vecSweepers[i].getStomach() == 1)
            {
                m_vecThePopulation1[car].dFitness = m_vecSweepers[i].Fitness();
                car++;
            }
            */
        }
        her = 0;
        car = 0;
    }

    //Another generation has been completed.

	//Time to run the GA and update the sweepers with their new NNs
	else
	{
		//update the stats to be used in our stat window
		m_vecAvFitness.push_back(m_pGA->AverageFitness());
		m_vecBestFitness.push_back(m_pGA->BestFitness());

        if (myfile.is_open())
        {
            for(int i = 0; i<m_NumSweepers; ++i)
            {
                myfile << "8";
                myfile << ", ";
                myfile << "30";
                myfile << ", ";
                myfile << "5";
                myfile << ", ";
                myfile << m_iGenerations;
                myfile << ", ";
                myfile << i;
                myfile << ", ";
                myfile << m_vecSweepers[i].getStomach();
                myfile << ", ";
                myfile << m_vecSweepers[i].getEnergy();
                myfile << ", ";
                myfile << m_vecSweepers[i].Fitness();
                myfile << ", ";
                myfile << (aveSpeed[i]/(CParams::iNumTicks - 1));
                myfile << ", ";
                myfile << (aveRotation[i]/(CParams::iNumTicks - 1));
                myfile << ", ";
                //myfile << (radians[i]/CParams::iNumTicks);
                float xy = atan2(yrad[i],xrad[i])*180/3.14159265;
                if(xy < 0){xy += 360;}
                myfile << xy;
                myfile << ", ";
                myfile << (aveMineDist[i]/CParams::iNumTicks);
                myfile << ", ";
                myfile << (aveBotDist[i]/CParams::iNumTicks);
                myfile << ", \n";

                aveSpeed[i] = 0;
                aveRotation[i] = 0;
                xrad[i] = 0;
                yrad[i] = 0;
                //radians[i] = 0;
                aveMineDist[i] = 0;
                aveBotDist[i] = 0;
            }
        }
		//increment the generation counter
		++m_iGenerations;

		//reset cycles
		m_iTicks = 0;

		//run the GA to create a new population
		m_vecThePopulation = m_pGA->Epoch(m_vecThePopulation);
		//m_vecThePopulation1 = m_pGA1->Epoch(m_vecThePopulation1);

		//insert the new (hopefully)improved brains back into the sweepers
    //and reset their positions etc
    her = 0;
    car = 0;
    for (int i=0; i<m_NumSweepers; ++i)
		{
		    if(m_vecSweepers[i].getStomach() == 0)
            {
                m_vecSweepers[i].PutWeights(m_vecThePopulation[her].vecWeights);
                m_vecSweepers[i].Reset();
                her++;
            }
            /*else if(m_vecSweepers[i].getStomach() == 1)
            {
                m_vecSweepers[i].PutWeights(m_vecThePopulation1[car].vecWeights);
                m_vecSweepers[i].Reset();
                car++;
            }*/

		}
	}
	return true;

}
//------------------------------------Render()--------------------------------------
//
//----------------------------------------------------------------------------------
void CController::Render(HDC surface)
{
	//render the stats
	string s = "Generation:          " + itos(m_iGenerations);
	TextOut(surface, 5, 0, s.c_str(), s.size());

	//do not render if running at accelerated speed
	if (!m_bFastRender)
	{
		//keep a record of the old pen
     m_OldPen = (HPEN)SelectObject(surface, m_GreenPen);

    //render the mines
		for (int i=0; i<m_NumMines; ++i)
		{
			//grab the vertices for the mine shape
			vector<SPoint> mineVB = m_MineVB;

			WorldTransform(mineVB, m_vecMines[i]);

			//draw the mines
			MoveToEx(surface, (int)mineVB[0].x, (int)mineVB[0].y, NULL);

			for (int vert=1; vert<mineVB.size(); ++vert)
			{
				LineTo(surface, (int)mineVB[vert].x, (int)mineVB[vert].y);
			}

			LineTo(surface, (int)mineVB[0].x, (int)mineVB[0].y);

		}

    //we want the fittest displayed in red
    SelectObject(surface, m_RedPen);

        int i;
		//render the sweepers
		for (i=0; i<m_NumSweepers; i++)
		{

			if (i == CParams::iNumElite)
      {
        SelectObject(surface, m_OldPen);
      }

        //Enable to have colours
        /*
      if (m_vecSweepers[i].getStomach() <= 0.2)
      {
          SelectObject(surface, m_IndigoPen);
      }

      else if (m_vecSweepers[i].getStomach() > 0.2 && m_vecSweepers[i].getStomach() <= 0.4)
      {
          SelectObject(surface, m_BluePen);
      }

      else if (m_vecSweepers[i].getStomach() > 0.4 && m_vecSweepers[i].getStomach() <= 0.6)
      {
          SelectObject(surface, m_GreenPen);
      }

      else if (m_vecSweepers[i].getStomach() > 0.6 && m_vecSweepers[i].getStomach() <= 0.8)
      {
          SelectObject(surface, m_OrangePen);
      }

      else if (m_vecSweepers[i].getStomach() > 0.8)
      {
          SelectObject(surface, m_RedPen);
      }
      */

      //grab the sweeper vertices
			vector<SPoint> sweeperVB = m_SweeperVB;

			//transform the vertex buffer
			m_vecSweepers[i].WorldTransform(sweeperVB);

			//draw entire bot
			MoveToEx(surface, (int)sweeperVB[0].x, (int)sweeperVB[0].y, NULL);

			for (int vert=1; vert<4; ++vert)
			{
				LineTo(surface, (int)sweeperVB[vert].x, (int)sweeperVB[vert].y);
			}
        //disable to remove numbers
      float st = m_vecSweepers[i].getStomach();
      st *= 100;
      st = (float)((int)st);
      st /= 100;
      ostringstream strs;
      strs << st;
      string str = strs.str();
      TextOut(surface, (int)sweeperVB[0].x, ((int)sweeperVB[0].y + 2), str.c_str(), str.size());

      LineTo(surface, (int)sweeperVB[0].x, (int)sweeperVB[0].y);
		}


    //put the old pen back
    SelectObject(surface, m_OldPen);

	}//end if

  else
  {
    PlotStats(surface);
  }

}
//--------------------------PlotStats-------------------------------------
//
//  Given a surface to draw on this function displays stats and a crude
//  graph showing best and average fitness
//------------------------------------------------------------------------
void CController::PlotStats(HDC surface)
{
    string s = "Best Fitness:       " + ftos(m_pGA->BestFitness());
	  TextOut(surface, 5, 20, s.c_str(), s.size());

     s = "Average Fitness: " + ftos(m_pGA->AverageFitness());
	  TextOut(surface, 5, 40, s.c_str(), s.size());

    //render the graph
    float HSlice = (float)cxClient/(m_iGenerations+1);
    float VSlice = (float)cyClient/((m_pGA->BestFitness()+1)*2);

    //plot the graph for the best fitness
    float x = 0;

    m_OldPen = (HPEN)SelectObject(surface, m_RedPen);

    MoveToEx(surface, 0, cyClient, NULL);

    for (int i=0; i<m_vecBestFitness.size(); ++i)
    {
       LineTo(surface, x, cyClient - VSlice*m_vecBestFitness[i]);

       x += HSlice;
    }

    //plot the graph for the average fitness
    x = 0;

    SelectObject(surface, m_BluePen);

    MoveToEx(surface, 0, cyClient, NULL);

    int i;
    for (i=0; i<m_vecAvFitness.size(); ++i)
    {
       LineTo(surface, (int)x, (int)(cyClient - VSlice*m_vecAvFitness[i]));

       x += HSlice;
    }

    //replace the old pen
    SelectObject(surface, m_OldPen);
}

