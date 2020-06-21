#include <string>
#include "MacroCxn.h"
#include "SimDefs.h"
#include "Network.h"
#include "RndNums.h"

MacroCxn::MacroCxn(QStringList tokens)
{
        RecID = 0;

        PreGrp = Network::GetGrp(tokens.at(1));
        PostGrp = Network::GetGrp(tokens.at(2));

        PostGrp->AddCxn(this);
        PostGrp->AddInCxn(this);
        PostGrp->AddInGrp(PreGrp);
        PreGrp->AddCxn(this);

        /* Tokens ordered by file specs */
        ID = tokens.at(3).toInt();
        cxn_type = tokens.at(4).toInt(0,16);
        //qDebug() << "TEST INPUT DEBUG: " << cxn_type << endl;
        /* cxn type numbers are encoded as 8-bits:
                b0: sensor input
                b1: coincident input
                b2: vd input
                b3: TBD
                b4: TBD
                b5: TBD
                b6: TBD
                b7: TBD
        */

        height = tokens.at(5).toInt();
        width = tokens.at(6).toInt();
        arbor = tokens.at(7);

        prob = tokens.at(8).toDouble();
        xoff = tokens.at(9).toInt();
        yoff = tokens.at(10).toInt();
        ang = tokens.at(11).toInt();

        //if cerebellum eventually
        infl.Set(tokens.at(14).toDouble(), 15, 16, true);
        // need to put plastic type towards the beginning so it's not
        // affected by +/- of other paramters that will offset
        // it's location
        plast_type = tokens.at(15).toInt();

        wmin = tokens.at(12).toDouble();
        wmax = tokens.at(13).toDouble();

        /* Default Values */
        VD = false;//voltage dependent
        sens = false;//sensor input
        coinc = false;//coincident input

        /* Set Connection Type */
        int sensMask = 0x01;//bit0
        int coincMask = 0x02;//bit1
        int vdMask = 0x04;//bit2
        if (cxn_type & sensMask) {
                sens = true;
        }
        if (cxn_type & coincMask) {
                coinc = true;
        }
        if (cxn_type & vdMask) {
                VD = true;
        }

        /* Default Values */
        D7_BCM = false; // could just called BCM and VAL D7 Plastictity
        D7_VAL = false;
        HIPPO_BCM = false;
        HIPPO_TD = false;
        CEREB_BCM = false;
        CEREB_TE = false;
        /* Set Plastic Type */
        switch(plast_type)
        {
                case 0:
                        //Non-Plastic
                        break;
                case 1:
                        D7_BCM = true;
                        break;
                case 2:
                        D7_VAL = true;
                        break;
                case 3:
                        HIPPO_BCM = true;
                        break;
                case 4:
                        HIPPO_TD = true;
                        break;
                case 5:
                        CEREB_BCM = true;
                        break;
                case 6:
                        CEREB_TE = true;
                        break;
                default:
                        break;
        }


        //#define WGT_DEBUGGER
        if (PostGrp->isMemSpaceVerifier())
        {
                // use plain integer for testing scenarios
                OriginalWeight.Set((int)wmin, 16, true);
        }
        else if (PostGrp->isDarwin7())
        {
                double wgtMin = wmin;
                double wgtMax = wmax;

                double wgtInRange = rnduniform(wmin, wmax);

                // radix = 15, range = (-1, 1), minimal increment => 2^-15 = 0.000030517578125
                OriginalWeight.Set(wgtInRange, 15, 16, true);

#ifdef WGT_DEBUGGER
                        qDebug() << "*****************  Weight Debugger  ***************** " << endl;
                        qDebug() << "Projection " << QString::fromStdString(PreGrp->GetName()) << " to"
                                        << QString::fromStdString(PostGrp->GetName()) << endl;
                        qDebug() << "Wgt Min " << wmin << " " << "Wgt Max " << wmax << endl;
                        qDebug() << "Random Weight in Range: " << wgtInRange << endl;
                        qDebug() << "Orignal Integer Weight: " << OriginalWeight.GetFxdVal() << endl
                                        << "Orignal Weight Scaled Back: "
                                        << OriginalWeight.GetFxdVal()*pow(2.0,-(double)OriginalWeight.GetRadix());
#endif
        }
        else if (PostGrp->isCerebellum())
        {
                double wgtMin = wmin;
                double wgtMax = wmax;
                double wgtInRange = rnduniform(wmin, wmax);

                // radix = 10, range = (-32, 32), minimal increment => 2^-10 = 0.0009765625 or ~0.001
                OriginalWeight.Set(wgtInRange, 10, 16, true);

#ifdef WGT_DEBUGGER
                        qDebug() << "*****************  Weight Debugger  ***************** " << endl;
                        qDebug() << "Projection " << QString::fromStdString(PreGrp->GetName()) << " to"
                                        << QString::fromStdString(PostGrp->GetName()) << endl;
                        qDebug() << "Wgt Min " << wmin << " " << "Wgt Max " << wmax << endl;
                        qDebug() << "Random Weight in Range: " << wgtInRange << endl;
                        qDebug() << "Orignal Integer Weight: " << hex << OriginalWeight.GetFxdVal() << endl
                                        << "Orignal Weight Scaled Back: "
                                        << OriginalWeight.GetFxdVal()*pow(2.0,-(double)OriginalWeight.GetRadix());
#endif
        }
        else if (PostGrp->isWeightReadWriter() || PostGrp->isWeightReader())
        {
                // use plain integer for testing scenarios
                OriginalWeight.Set(1, 16, true);
        }
        else
        {
                // use plain integer for testing scenarios
                OriginalWeight.Set((int)wmin, 16, true);
        }

        /* Default Values */
        plast_wmax.Set(0.0, 14, 16, true);
        learning_rate.Set(0.0, 15, 16, true);
        decay.Set(0.0, 15, 16, true);
        theta1.Set(0.0,15,16,true);
        theta2.Set(0.0,15,16,true);
        k1.Set(0.0,15,16,true);
        k2.Set(0.0,15,16,true);
        sat.Set(0.0,15,16,true);

        trace_intern_init.Set(0,15,false);
        trace_intern_rate.Set(0,15,false);
        trace_intern_thresh.Set(0,15,false);
        trace_elig_rate.Set(0.0,8,8,false);
        trace_elig_min.Set(0.0,8,8,false);
        trace_input_thresh.Set(0.0,8,8,false);
        //trace_motorlearn_penal = 0.0;
        ValGrp = NULL;

        //Special Test Format*
        if (PostGrp->isMemSpaceVerifier())
        {
                plast_wmax.Set(tokens.at(16).toInt(),16,true);
                learning_rate.Set(tokens.at(17).toInt(),16,true);
                decay.Set(tokens.at(18).toInt(),16,true);
                theta1.Set(tokens.at(19).toInt(),16,true);
                theta2.Set(tokens.at(20).toInt(),16,true);
                k1.Set(tokens.at(21).toInt(),16,true);
                k2.Set(tokens.at(22).toInt(),16,true);
                sat.Set(tokens.at(23).toInt(),16,true);

                theta12_Avg.Set(8, 16, true);
                K2_Saturation_Quotient.Set(9, 16, true);
        }
        //FORMAT 1
        else if (D7_BCM || D7_VAL || HIPPO_BCM || CEREB_BCM)
        {
                plast_wmax.Set(tokens.at(16).toDouble(),15,16,true);
                learning_rate.Set(tokens.at(17).toDouble(),15,16,true);
                decay.Set(tokens.at(18).toDouble(),15,16,true);
                theta1.Set(tokens.at(19).toDouble(),15,16,true);
                theta2.Set(tokens.at(20).toDouble(),15,16,true);
                k1.Set(tokens.at(21).toDouble(),15,16,true);
                k2.Set(tokens.at(22).toDouble(),15,16,true);
                sat.Set(tokens.at(23).toInt(),15,false);
                if (D7_VAL) {
                        ValGrp = Network::GetGrp(tokens.at(24));
                }

                double k2_temp = tokens.at(22).toDouble();
                double sat_temp = tokens.at(23).toDouble();
                double k2_sat_quot_temp = k2_temp/sat_temp;

                double theta1_temp = tokens.at(19).toDouble();
                double theta2_temp = tokens.at(20).toDouble();
                double theta12Avg_temp = (theta1_temp + theta2_temp)/2.0;

                K2_Saturation_Quotient.Set(k2_sat_quot_temp, 15, 16, true);
                theta12_Avg.Set(theta12Avg_temp, 15, 16, true);
        }
        //FORMAT 2
        else if (CEREB_TE)
        {
                plast_wmax.Set(tokens.at(16).toDouble(),10,16,true);
                learning_rate.Set(tokens.at(17).toDouble(),15,16,true);

                trace_intern_init.Set(tokens.at(18).toInt(),15,false);
                trace_intern_rate.Set(tokens.at(19).toInt(),15,false);

                trace_elig_rate.Set(tokens.at(20).toDouble(),8,8,false);
                trace_elig_min.Set(tokens.at(21).toDouble(),8,8,false);

                trace_intern_thresh.Set(tokens.at(22).toInt(),15,false);
                trace_input_thresh.Set(tokens.at(23).toDouble(),8,8,false);

                ValGrp = Network::GetGrp(tokens.at(24));
        }
}

bool MacroCxn::isPlastic()
{
        if (D7_VAL || HIPPO_TD || CEREB_TE || D7_BCM || HIPPO_BCM || CEREB_BCM)
        {
                return true;
        }
        else{
                return false;
        }
}

bool MacroCxn::isVal()
{
        if (D7_VAL || HIPPO_TD || CEREB_TE)
        {
                return true;
        }
        else{
                return false;
        }
}

MacroCxn::~MacroCxn()
{
        for (MicroLstIter it = MicroCxns.begin();
                it != MicroCxns.end(); ++it)
        {
                delete *it;
        }
}