#include "CMinesweeper.h"
#include <time.h>

//-----------------------------------constructor-------------------------
//
//-----------------------------------------------------------------------
CMinesweeper::CMinesweeper():
                             m_dRotation(RandFloat()*CParams::dTwoPi),
                             m_lTrack(0.16),
                             m_rTrack(0.16),
                             m_blTrack(0.16),
                             m_brTrack(0.16),
                             m_dFitness(0),
                             m_dScale(CParams::iSweeperScale),
                             m_iClosestMine(0),
                             m_iClosestBot(0)

{
	//create a random start position
	m_vPosition = SVector2D((RandFloat() * CParams::WindowWidth),
					                (RandFloat() * CParams::WindowHeight));
    m_Energy = 50.0;
    //m_Energy = (RandFloat() * 100);
    //stomachType = RandFloat();
    //stomachType = rand()%4;
    //if(stomachType == 2 || stomachType == 3){stomachType = 0;}
    stomachType = 0;

}

//-------------------------------------------Reset()--------------------
//
//	Resets the sweepers position, fitness and rotation
//
//----------------------------------------------------------------------
void CMinesweeper::Reset()
{
	//reset the sweepers positions
	m_vPosition = SVector2D((RandFloat() * CParams::WindowWidth),
					                (RandFloat() * CParams::WindowHeight));

	//and the fitness
	m_dFitness = 0;

    //and the rotation
    m_dRotation = RandFloat()*CParams::dTwoPi;
    m_Energy = 50;
    //m_Energy = (RandFloat() * 100);

	return;
}
//---------------------WorldTransform--------------------------------
//
//	sets up a translation matrix for the sweeper according to its
//  scale, rotation and position. Returns the transformed vertices.
//-------------------------------------------------------------------
void CMinesweeper::WorldTransform(vector<SPoint> &sweeper)
{
	//create the world transformation matrix
	C2DMatrix matTransform;

	//scale
	matTransform.Scale(m_dScale, m_dScale);

	//rotate
	matTransform.Rotate(m_dRotation);

	//and translate
	matTransform.Translate(m_vPosition.x, m_vPosition.y);

	//now transform the ships vertices
	matTransform.TransformSPoints(sweeper);
}

//-------------------------------Update()--------------------------------
//
//	First we take sensor readings and feed these into the sweepers brain.
//
//	The inputs are:
//
//	A vector to the closest mine (x, y)
//	The sweepers 'look at' vector (x, y)
//
//	We receive two outputs from the brain.. lTrack & rTrack.
//	So given a force for each track we calculate the resultant rotation
//	and acceleration and apply to current velocity vector.
//
//-----------------------------------------------------------------------
bool CMinesweeper::Update(vector<SVector2D> &mines, vector<SVector2D> &bots, vector<CMinesweeper> &bots2)
{

	//this will store all the inputs for the NN
	vector<double> inputs;

	//get vector to closest mine
	SVector2D vClosestMine = GetClosestMine(mines);

	//normalise it
  Vec2DNormalize(vClosestMine);

    SVector2D vClosestBot = GetClosestBot(bots);
    Vec2DNormalize(vClosestBot);

    double m_dClosestStomach = CheckStomach(bots2);
    double myStomach = CMinesweeper().getStomach();

  //add in vector to closest mine

    inputs.push_back(0/*vClosestMine.x*/); // set to 0 for blind to food
    inputs.push_back(0/*vClosestMine.y*/);

/*
    else if(stomachType == 1)
    {
        inputs.push_back(0);//predators cant see mines
        inputs.push_back(0);
    }
*/

	//add in sweepers look at vector
	inputs.push_back(0/*m_vLookAt.x*/);//set to 0 for no head direction
	inputs.push_back(0/*m_vLookAt.y*/);

	inputs.push_back(0/*vClosestBot.x*/); //set to 0 for no bot
	inputs.push_back(0/*vClosestBot.y*/);

	inputs.push_back(0/*m_dClosestStomach*/);
	inputs.push_back(0/*myStomach*/);

	//update the brain and get feedback
	vector<double> output = m_ItsBrain.Update(inputs);

	//make sure there were no errors in calculating the
	//output
	if (output.size() < CParams::iNumOutputs)
  {
    return false;
  }

	//assign the outputs to the sweepers left & right tracks
	m_lTrack = output[0];
	m_rTrack = output[1];
	m_blTrack = output[2];
	m_brTrack = output[3];

	//calculate steering forces
	double RotForce = m_lTrack - (0.8 * m_blTrack) + (0.8 * m_brTrack) - m_rTrack;

	//clamp rotation
	Clamp(RotForce, -CParams::dMaxTurnRate, CParams::dMaxTurnRate);


/*
    if(stomachType == 1)
    {
        m_dSpeed = (m_lTrack + m_rTrack - (0.8 * m_blTrack) - (0.8 * m_brTrack)) * 2;
        m_dRotation += RotForce*2;
    }
	else if(stomachType == 0)
    {
        m_dSpeed = (m_lTrack + m_rTrack - (0.8 * m_blTrack) - (0.8 * m_brTrack));
        m_dRotation += RotForce;
    }
    */
    m_dSpeed = (m_lTrack + m_rTrack - (0.8 * m_blTrack) - (0.8 * m_brTrack));
    m_dRotation += RotForce;

	//update Look At
	m_vLookAt.x = -sin(m_dRotation);
	m_vLookAt.y = cos(m_dRotation);

	//update position
  m_vPosition += (m_vLookAt * m_dSpeed);

	//wrap around window limits
	if (m_vPosition.x > CParams::WindowWidth) m_vPosition.x = 0;
	if (m_vPosition.x < 0) m_vPosition.x = CParams::WindowWidth;
	if (m_vPosition.y > CParams::WindowHeight) m_vPosition.y = 0;
	if (m_vPosition.y < 0) m_vPosition.y = CParams::WindowHeight;

	return true;
}


//----------------------GetClosestObject()---------------------------------
//
//	returns the vector from the sweeper to the closest mine
//
//-----------------------------------------------------------------------
SVector2D CMinesweeper::GetClosestMine(vector<SVector2D> &mines)
{
	double			closest_so_far = 99999;

	SVector2D		vClosestObject(0, 0);

	//cycle through mines to find closest
	for (int i=0; i<mines.size(); i++)
	{
		double len_to_object = Vec2DLength(mines[i] - m_vPosition);

		if (len_to_object < closest_so_far)
		{
			closest_so_far	= len_to_object;

			vClosestObject	= m_vPosition - mines[i];

      m_iClosestMine = i;
		}
		vecMinex = vClosestObject.x;
		vecMiney = vClosestObject.y;
	}

	return vClosestObject;
}

SVector2D CMinesweeper::GetClosestBot(vector<SVector2D> &bots) //add in colour of bot
{
    double closest_so_far = 99999;
    SVector2D vClosestObject(0, 0);
    //cycle through bots to find closest
	for (int i=0; i<bots.size(); i++)
	{
		double len_to_object = Vec2DLength(bots[i] - m_vPosition);

		if (len_to_object != 0 && len_to_object < closest_so_far)
		{
			closest_so_far	= len_to_object;

			vClosestObject	= m_vPosition - bots[i];

      m_iClosestBot = i;
		}
		vecBotx = vClosestObject.x;
		vecBoty = vClosestObject.y;
	}
	return vClosestObject;
}

//----------------------------- CheckForMine -----------------------------
//
//  this function checks for collision with its closest mine (calculated
//  earlier and stored in m_iClosestMine)
//-----------------------------------------------------------------------
int CMinesweeper::CheckForMine(vector<SVector2D> &mines, double size)
{
  SVector2D DistToObject = m_vPosition - mines[m_iClosestMine];

	if (Vec2DLength(DistToObject) < (size + 5))
	{
			return m_iClosestMine;
  }

  return -1;
}


int CMinesweeper::CheckForBot(vector<SVector2D> &bots, double size)
{
  SVector2D DistToBot = m_vPosition - bots[m_iClosestBot];

	if (Vec2DLength(DistToBot) <= ((float)size + 5)) //making size larger increases best fitness, larger range
	{
			return m_iClosestBot;
  }

  return -1;
}

int CMinesweeper::CheckEnergy(vector<CMinesweeper> &bots2)
{
			return bots2[m_iClosestBot].getEnergy();
}

double CMinesweeper::CheckStomach(vector<CMinesweeper> &bots2)
{
			return bots2[m_iClosestBot].getStomach();
}
