/**
 * @file MacroCxn.h
 *
 * A Macro-Connection represents a high-level
 * synapse type between two neuronal Groups described
 * in the Neuro-Anatomy file.The MacroCxn class
 * holds all the coefficients and properties used
 * in network initialization, cell activity calculation,
 * and cell connection update processes.
 */
#ifndef MACROCXN_H_
#define MACROCXN_H_

#include <QString>
#include "Group.h"
#include "Coeff.h"

using namespace std;

/*Forward Declaration*/
class Group;

/**
 * A Macro-Connection represents a chunk of synapses
 * between two Groups.
 */
class MacroCxn {
public:
   MacroCxn( QStringList tokens );
   ~MacroCxn();

   /* Return the total number of connections in the type */
   int GetNumCxns() { return (int)MicroCxns.size(); }

   void AddMicro( MicroCxn* NewMicro ) { MicroCxns.push_back( NewMicro ); }
   bool isPostGrp(Group* grp) { return grp == PostGrp; }
   bool isPreGrp(Group* grp) { return grp == PreGrp; }

   /* Get Parameters */
   Group* GetPreGrp() { return PreGrp; }
   Group* GetPostGrp() { return PostGrp; }
   Group* GetValGrp() { return ValGrp; }

   int GetID(){ return ID; }
   QString GetArbor() { return arbor; }
   float GetProb() { return prob; }
   int GetHeight() { return height; }
   int GetWidth() { return width; }
   int GetXoff() { return xoff; }
   int GetYoff() { return yoff; }
   int GetAng() { return ang; }
   double GetWmax() { return wmax; }
   double GetWmin() { return wmin; }
   int GetCxnType() { return cxn_type; }
   int GetPlastType() { return plast_type; }

   Coeff GetLearningRate() { return learning_rate; }
   Coeff GetPlastWmax() { return plast_wmax; }
   Coeff GetOriginalWeight() { return OriginalWeight; }
   Coeff GetDecay() { return decay; }
   Coeff GetK1() { return k1; }
   Coeff GetK2() { return k2; }
   Coeff GetTheta1() { return theta1; }
   Coeff GetTheta2() { return theta2; }
   Coeff GetSaturation() { return sat; }
   Coeff GetInfl() { return infl; }
   Coeff GetTheta12Avg() { return theta12_Avg; }
   Coeff GetK2SaturationQuotient( ){ return K2_Saturation_Quotient; }

   Coeff GetInternInit() { return trace_intern_init; }
   Coeff GetInternRate() { return trace_intern_rate; }
   Coeff GetEligRate() { return trace_elig_rate; }
   Coeff GetEligMin() { return trace_elig_min; }
   Coeff GetInternThresh() { return trace_intern_thresh; }
   Coeff GetInputThresh() { return trace_input_thresh; }
   //float GetMotorPenal(){ return trace_motorlearn_penal; }

   bool isPlastic();
   bool isVal();
   bool isVD() { return VD; }
   bool isSens() { return sens; }
   bool isCoinc() { return coinc; }

   bool isDarwin7_Plasticity() { return (D7_BCM || D7_VAL); }
   bool isCerebellar_ValueDependent() { return CEREB_TE; }
   bool isHippocampus() { return HIPPO_BCM || HIPPO_TD; }

   int GetSynRecID() { return RecID; }
   void SetSynRecID(int rec) { RecID = rec; }

private:
   /* The actual connections */
   MicroLst MicroCxns;
   /* Synapse Characteristics */
   bool VD;
   bool sens;
   bool coinc;

   bool D7_BCM;
   bool D7_VAL;
   bool HIPPO_BCM;
   bool HIPPO_TD;
   bool CEREB_BCM;
   bool CEREB_TE;

   int RecID; //parameter record ID for OnChip Group-Assoc. table

   /* connection parameters */
   Group* PostGrp; /* The Group that receives the input */
   Group* PreGrp;  /* The Group that provides the input */
   Group* ValGrp;

   int ID; // Anatomy system ID
   QString arbor; /**< Connection type that follows specific rules as defined in Network::Synapse() */
   double prob;
   int height;
   int width;
   int xoff;
   int yoff;
   int ang;
   double wmin;
   double wmax;
   int cxn_type;
   int plast_type;

   Coeff infl;
   Coeff plast_wmax;
   Coeff learning_rate;
   Coeff OriginalWeight;

   /* Format 1: BCM plasticity parameters */
   Coeff decay;
   Coeff theta1;
   Coeff theta2;
   Coeff k1;
   Coeff k2;
   Coeff sat;
   Coeff theta12_Avg;
   Coeff K2_Saturation_Quotient;

   /* Format 2: TE plasticity parameters */
   Coeff trace_intern_init;
   Coeff trace_intern_rate;
   Coeff trace_elig_rate;
   Coeff trace_elig_min;
   Coeff trace_intern_thresh;
   Coeff trace_input_thresh;
   //float trace_motorlearn_penal;

   /* Parameters added on 'x' card for Izhikevich test */
   int cnopt;
   int nc;
   int et;
   int sjrev;
   int cijmask;
   Coeff scl;
   };
#endif