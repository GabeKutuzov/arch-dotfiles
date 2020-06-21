#include <QtCore>
#include <list>
#include <exception>

#include "Network.h"
#include "Group.h"
#include "RndNums.h"
#include "SimDefs.h"
#include "MicroCxn.h"
#include "MacroCxn.h"
#include "OutputWin.h"
#include "SPP2Globals.h"

using namespace std;


/* Debug */
//int SpecialCounter = 0;

// need to add to future static class
double square(double x) {
        return x*x;
}

int Network::TotalCells()
{
        int NumCells = 0;
        for (list<Group*>::iterator it = Groups.begin();
                        it != Groups.end(); ++it)
        {
                NumCells += (*it)->GetSize();
        }
        return NumCells;
}

int Network::TotRegCells()
{
        int NumCells = 0;
        for (list<Group*>::iterator it = Groups.begin();
                        it != Groups.end(); ++it)
        {
                if (!(*it)->isSens()) {
                        NumCells += (*it)->GetSize();
                }
        }
        return NumCells;
}

int Network::TotSensCells()
{
        int NumCells = 0;
        for (list<Group*>::iterator it = Groups.begin();
                        it != Groups.end(); ++it)
        {
                if ((*it)->isSens()) {
                        NumCells += (*it)->GetSize();
                }
        }
        return NumCells;
}

int Network::TotValCells()
{
        int NumCells = 0;
        for (list<Group*>::iterator it = Groups.begin();
                        it != Groups.end(); ++it)
        {
                if ((*it)->isVal()) {
                        NumCells += (*it)->GetSize();
                }
        }
        return NumCells;
}

int Network::TotalCxns()
{
        return TotCxns;
}

int Network::TotalPlast()
{
        int plast = 0;
        for (list<Group*>::iterator it = Groups.begin();
                        it != Groups.end(); ++it)
        {
                plast += (*it)->GetNumPlast();
        }
        return plast;
}

double Network::AvgCxns()
{
        return (double)TotalCxns()/(double)TotRegCells();
}

double Network::AvgPlast()
{
        return (double)TotalPlast()/(double)TotRegCells();
}

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//@                                                                            @
//@     Title:  BOOL Network_Synapse (int proj_height, int proj_width,         @
//@                                int from_height, int from_width,            @
//@                                int to_height, int to_width                 @
//@                                int from_inx, int to_inx                    @
//@                                char *proj_type, double prob                @
//@                                                                            @
//@     Action: Returns true if a synapse can be made between the two cell     @
//@             indices.  Projection types are made to the postsynaptic side.  @
//@             The following projection types are supported:                  @
//@                                                                            @
//@                nontopo - Nontopographic. Two cells have a uniform chance,  @
//@                   given by prob, of forming a synapse.                     @
//@                blocku - Block uniform. Two cells that are in the same      @
//@                   block of the cell group (given by the projection height  @
//@                   and projection width) have a uniform chance, given by    @
//@                   prob, of forming a synapse.                              @
//@                blocku_no - Sane as blocku with no wrap around              @
//@                blocku_ref - Sane as blocku with reflect effects.           @
//@                horizu - Horizontal uniform. Two cells whose distance       @
//@                   horizontally lies between proj_height and proj_width     @
//@                   have a uniform chance, given by prob, of forming a       @
//@                   synapse.                                                 @
//@                vertiu - Vertical uniform. Two cells whose distance         @
//@                   vertically lies between proj_height and proj_width       @
//@                   have a uniform chance, given by prob, of forming a       @
//@                   synapse.                                                 @
//@                leftu - Left uniform. The presynaptic cell synapses cells   @
//@                   on the left side of the postsynaptic group by the        @
//@                   same rule as blocku.                                     @
//@                rightu - Right uniform. The presynaptic cell synapses cells @
//@                   on the right side of the postsynaptic group by the       @
//@                   same rule as blocku.                                     @
//@                centeru - Center Uniform. Two cells whose distance          @
//@                   apart lies within a radius given by proj_height          @
//@                   have a uniform chance, given by prob, of forming a       @
//@                   synapse.                                                 @
//@                surroundu - Surround Uniform. Two cells whose distance      @
//@                   apart lies between two radii (the first given by         @
//@                   proj_height and the second given by proj_width have a    @
//@                   uniform chance, given by prob, of forming a synapse.     @
//@                centerg - Center Gaussian. Two cells whose distance         @
//@                   apart lies within a 2 dimensional gaussian (with a       @
//@                   standard deviation of proj_height) have a uniform        @
//@                   chance, given by prob, of forming a synapse.             @
//@                surroundg - Surround Gaussian. Two cells whose distance     @
//@                   apart lies between two gaussians (the first with a       @
//@                   standard deviation of proj_height and the second with a  @
//@                   standard deviation of proj_width) have a uniform chance, @
//@                   given by prob, of forming a synapse.                     @
//@                userspec - User specified projection file. The projection   @
//@                   type contains a file name with cell pairs specifying     @
//@                   a synapse.                                               @
//@                                                                            @
//@     Input:  proj_height - projection height for blocku, radius for centeru,@
//@                           smaller radius for surroundu, and standard       @
//@                           deviation for centerg and surroundg.             @
//@             proj_width - projection width for blocku, larger radius for    @
//@                          surroundu, and standard deviation for surroundg.  @
//@             from_height - height of the presynaptic group.                 @
//@             from_width - width of the presynaptic group.                   @
//@             to_height - height of the postsynaptic group.                  @
//@             to_width - width of the postsynaptic group.                    @
//@             from_inx - index of the presynaptic cell.                      @
//@             to_inx - index of the postsynaptic cell.                       @
//@             proj_type - projection type. can be nontopo, blocku, centeru,  @
//@                         surroundu, centerg, blockg, userspec.              @
//@             prob - probability of a synapse if the two cells pass the      @
//@                    projection type criteria.                               @
//@     Output: true if synapse between from_inx and to_inx.                   @
//@                                                                            @
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
bool Network::Synapse(int proj_height, int proj_width, int proj_xoffset,
   int proj_yoffset, int proj_angle, int from_height, int from_width,
   int to_height, int to_width, int from_inx, int to_inx,
   QString proj_type, double prob) {

        bool synapse = false;
        double from_x, from_y, to_x, to_y;
        double scale_height;
        double scale_width;
        double dist, distx, disty;
        double r;
        float inv_tan, angle;
        float tolerance = 5;

        //#define PROJECTION_DEBUG

        /* NewMacro->GetHeight(), NewMacro->GetWidth(), NewMacro->GetXoff(),
        *  NewMacro->GetYoff(), NewMacro->GetAng(),
        *  NewMacro->GetPreGrp()->GetHeight(), 
        *  NewMacro->GetPreGrp()->GetWidth(),
        *  NewMacro->GetPostGrp()->GetHeight(),
        *  NewMacro->GetPostGrp()->GetWidth(),
        *  i, j, NewMacro->GetArbor(), NewMacro->GetProb()) */

        //**********************************************************************
        // NOTE: All connection types that do not depend on the connection
        // probability should be placed here, above the orobability test below.

#ifdef USERSPEC_
                // user specified projection
                if (proj_type.compare ("userspec", Qt::CaseInsensitive) == 0) {
                        if (userspec_ary [from_inx*to_height*to_width+to_inx]) {
                                synapse = true;
                        }
                        //*****  EARLY exit!! ******
                        return synapse;
                }
#endif



        if (rnd01() >= prob) {
                //*****  EARLY exit for efficiency!!! ******
                // ALL connection types, except user defined, perform this test.
                // Therefore, there is no sense performing calculations just to
                // find out that you weren't going to create a connection anyway.
                // This way, we only do work if we are going to have a connection.
                // Given that the connection probability is often times low,
                // This should save a lot of time initializing the network.
                return synapse; // FALSE.
        }
        // postcondition: rnd01() < prob.

        // non-topographical uniformly random projection
        if (proj_type.compare("nontopo", Qt::CaseInsensitive) == 0)
        {
#ifdef PROJECTION_DEBUG
         qDebug() << "Arbor: " << QString::fromStdString(proj_type) << endl;
#endif

                // If rnd01() >= prob, we would have already exited.
                synapse = true;
                //*****  EARLY exit for efficiency!!! ******
                return synapse;
        }

        scale_height = (double)to_height / (double)from_height;
        scale_width = (double)to_width / (double)from_width;

        // make the index of the sending cell to be the same
        // scale as the receiving cell
        from_x = (double)(from_inx % from_width) * scale_width;
        from_y = (double)(from_inx / from_width) * scale_height;

        to_x = (double)(to_inx % to_width);
        to_y = (double)(to_inx / to_width);

        angle = (float)((float)proj_angle/180.0)*PI;
        tolerance = (float)(tolerance/180.0)*PI;

        if (proj_type.compare ("user_define", Qt::CaseInsensitive) == 0) {
                distx = fabs ((from_x + proj_xoffset) - to_x);
                disty = fabs ((from_y + proj_yoffset) - to_y);

                if (proj_angle == 0) {
                       if ((distx < (double)proj_width+0.5+ZERO) && (disty < (double)proj_height+0.5+ZERO)) {
                                synapse = true;
                       }
                }
                else if ((proj_angle > 0) && (proj_angle < 90)) {
                        if (((to_x >= (from_x+proj_xoffset))&&(to_y >= (from_y+proj_yoffset)))  ||
                                        ((to_x <= (from_x+proj_xoffset))&&(to_y <= (from_y+proj_yoffset)))) {
                                if ((disty < (proj_width*sin(angle))) && (distx < (proj_width*cos(angle)))) {
                                        inv_tan = atan((float)disty/(float)distx);
                                        if ((inv_tan > (fabs(angle)-tolerance)) && (inv_tan < (fabs(angle)+tolerance))) {
                                                synapse = true;
                                        }
                                }
                        }
                }
                else if ((proj_angle > 90) && (proj_angle < 180)) {
                        if (((to_x >= (from_x+proj_xoffset))&&(to_y <= (from_y+proj_yoffset)))  ||
                                        ((to_x <= (from_x+proj_xoffset))&&(to_y >= (from_y+proj_yoffset)))) {
                                if ((disty < (fabs(proj_width*sin(angle)))) && (distx < (fabs(proj_width*cos(angle))))) {
                                        inv_tan = atan((float)disty/(float)distx);
                                        if ((inv_tan > ((angle-(PI/2))-tolerance)) && (inv_tan < ((angle-(PI/2))+tolerance))) {
                                                synapse = true;
                                        }
                                }
                        }
                }

        }

        // block uniform projection
        else if (proj_type.compare ("blocku", Qt::CaseInsensitive) == 0)
        {
#ifdef PROJECTION_DEBUG
                        qDebug() << "Arbor: " << QString::fromStdString(proj_type) << endl;
#endif
                distx = fabs (from_x - to_x);

                // check for right wraparound
                if (fabs (from_x - (to_x + to_width)) < distx) {
                        distx = fabs (from_x - (to_x + to_width));
                }
                // check for left wraparound
                else if (fabs((from_x + to_width) - to_x) < distx) {
                        distx = fabs((from_x + to_width) - to_x);
                }
                disty = fabs (from_y - to_y);

                // check for bottom wraparound
                if (fabs (from_y - (to_y + to_height)) < disty) {
                        disty = fabs (from_y - (to_y + to_height));
                }
                // check for top wraparound
                else if (fabs((from_y + to_height) - to_y) < disty) {
                        disty = fabs((from_y + to_height) - to_y);
                }

                if ((distx < (double)(proj_width)+0.5+ZERO) && (disty < (double)(proj_height)+0.5+ZERO)) {
                        synapse = true;
                }

        }
        else if (proj_type.compare ("blocku_no" , Qt::CaseInsensitive) == 0)
        {
#ifdef PROJECTION_DEBUG
                        qDebug() << "Arbor: " << QString::fromStdString(proj_type) << endl;
#endif
                distx = fabs (from_x - to_x);
                disty = fabs (from_y - to_y);

                if ((distx < (double)(proj_width)+0.5+ZERO) && (disty < (double)(proj_height)+0.5+ZERO))
                {
                        synapse = true;
                }
        }
        else if (proj_type.compare ("blocku_no_left" , Qt::CaseInsensitive) == 0) {

                distx = fabs (from_x - to_x);

                if (fabs(from_y - to_y)<0.0001f && (to_x <= from_x)) {
                        if ((distx < (double)(proj_width)+0.5+ZERO)&&(distx > (double)(proj_height)+0.5+ZERO)) {
                                synapse = true;
                        }
                }
        }
        else if (proj_type.compare ("blocku_no_right" , Qt::CaseInsensitive) == 0) {

                distx = fabs (from_x - to_x);

                if (fabs(from_y - to_y)<0.0001f && (to_x >= from_x)) {
                        if ((distx < (double)(proj_width)+0.5+ZERO)&&(distx > (double)(proj_height)+0.5+ZERO)) {
                                synapse = true;
                        }
                }
        }
        else if (proj_type.compare ("blocku_no_up" , Qt::CaseInsensitive) == 0) {

                disty = fabs (from_y - to_y);

                if (fabs(from_x - to_x)<0.0001f && (to_y >= from_y)) {
                        if ((disty < (double)(proj_width)+0.5+ZERO)&&(disty > (double)(proj_height)+0.5+ZERO)) {
                                synapse = true;
                        }
                }
        }
        else if (proj_type.compare ("blocku_no_down" , Qt::CaseInsensitive) == 0) {

                disty = fabs (from_y - to_y);

                if (fabs(from_x - to_x)<0.0001f && (to_y <= from_y)) {
                        if ((disty < (double)(proj_width)+0.5+ZERO)&&(disty > (double)(proj_height)+0.5+ZERO)) {
                                synapse = true;
                        }
                }
        }
        else if (proj_type.compare ("blocku_no_dl" , Qt::CaseInsensitive) == 0) {

                distx = fabs (from_x - to_x);
                disty = fabs (from_y - to_y);

                if (fabs(distx - disty)<0.0001f && (to_x <= from_x) && (to_y <= from_y)) {
                        if ((distx < (double)(proj_width)+0.5+ZERO) && (disty < (double)(proj_height)+0.5+ZERO)) {
                                synapse = true;
                        }
                }
        }
        else if (proj_type.compare ("blocku_no_dr" , Qt::CaseInsensitive) == 0) {

                distx = fabs (from_x - to_x);
                disty = fabs (from_y - to_y);

                if (fabs(distx - disty)<0.0001f && (to_x >= from_x) && (to_y <= from_y)) {
                        if ((distx < (double)(proj_width)+0.5+ZERO) && (disty < (double)(proj_height)+0.5+ZERO)) {
                                synapse = true;
                        }
                }
        }
        else if (proj_type.compare ("blocku_no_ul" , Qt::CaseInsensitive) == 0) {

                distx = fabs (from_x - to_x);
                disty = fabs (from_y - to_y);

                if (fabs(distx - disty)<0.0001f && (to_x <= from_x) && (to_y >= from_y)) {
                        if ((distx < (double)(proj_width)+0.5+ZERO) && (disty < (double)(proj_height)+0.5+ZERO)) {
                                synapse = true;
                        }
                }
        }
        else if (proj_type.compare ("blocku_no_ur" , Qt::CaseInsensitive) == 0) {

                distx = fabs (from_x - to_x);
                disty = fabs (from_y - to_y);

                if (fabs(distx - disty)<0.0001f && (to_x >= from_x) && (to_y >= from_y)) {
                        if ((distx < (double)(proj_width)+0.5+ZERO) && (disty < (double)(proj_height)+0.5+ZERO)) {
                                synapse = true;
                        }
                }
        }
        else if (proj_type.compare ("diaguR_no" , Qt::CaseInsensitive) == 0) {

                distx = fabs (from_x - to_x);
                disty = fabs (from_y - to_y);

                if (((from_x > to_x) && (from_y > to_y)) || ((from_x < to_x) && (from_y < to_y))) {
                        if ((int)distx == (int)disty) {
                                if ((distx < (double)(proj_width)+0.5+ZERO) && (disty < (double)(proj_height)+0.5+ZERO)) {
                                        synapse = true;
                                }
                        }
                }
        }
        else if (proj_type.compare ("diaguL_no" , Qt::CaseInsensitive) == 0) {

                distx = fabs (from_x - to_x);
                disty = fabs (from_y - to_y);

                if (((from_x > to_x) && (from_y < to_y)) || ((from_x < to_x) && (from_y > to_y))) {
                        if ((int)distx == (int)disty) {
                                if ((distx < (double)(proj_width)+0.5+ZERO) && (disty < (double)(proj_height)+0.5+ZERO)) {
                                        synapse = true;
                                }
                        }
                }
        }
        else if (proj_type.compare ("blocku_no" , Qt::CaseInsensitive) == 0) {

                distx = fabs (from_x - to_x);
                disty = fabs (from_y - to_y);

                if ((distx < (double)(proj_width)+0.5+ZERO) && (disty < (double)(proj_height)+0.5+ZERO))
                {
                        synapse = true;
                }
        }

        // left only from -     only project from the left side of the presynaptic neural area
        //                      no wraparound
        else if (proj_type.compare ("leftonlyfrm" , Qt::CaseInsensitive) == 0) {

                distx = fabs (from_x - to_x);
                disty = fabs (from_y - to_y);

                if ((distx < (double)(proj_width)+0.5+ZERO) && (disty < (double)(proj_height)+0.5+ZERO) &&
                    (from_x < to_width/2)) {
                        synapse = true;
                }
        }
        // right only from -    only project from the right side of the presynaptic neural area
        //                      no wraparound
        else if (proj_type.compare ("rightonlyfrm" , Qt::CaseInsensitive) == 0) {

                distx = fabs (from_x - to_x);
                disty = fabs (from_y - to_y);

                if ((distx < (double)(proj_width)+0.5+ZERO) && (disty < (double)(proj_height)+0.5+ZERO) &&
                    (from_x > to_width/2)) {
                        synapse = true;
                }
        }
        // left only to -       only project to the left side of the postsynaptic neural area
        //                      no wraparound
        else if (proj_type.compare ("leftonlyto" , Qt::CaseInsensitive) == 0) {

                distx = fabs (from_x - to_x);
                disty = fabs (from_y - to_y);

                if ((distx < (double)(proj_width)+0.5+ZERO) && (disty < (double)(proj_height)+0.5+ZERO) &&
                    (to_x < to_width/2)) {
                        synapse = true;
                }
        }
        // right only to -      only project to the right side of the postsynaptic neural area
        //                      no wraparound
        else if (proj_type.compare ("rightonlyto" , Qt::CaseInsensitive) == 0) {

                distx = fabs (from_x - to_x);
                disty = fabs (from_y - to_y);

                if ((distx < (double)(proj_width)+0.5+ZERO) && (disty < (double)(proj_height)+0.5+ZERO) &&
                    (to_x > to_width/2)) {
                        synapse = true;
                }
        }
        // horizontal uniform projection
        else if (proj_type.compare ("horizu" , Qt::CaseInsensitive) == 0) {

                distx = fabs (from_x - to_x);

                // check for right wraparound
                if (fabs (from_x - (to_x + to_width)) < distx) {
                        distx = fabs (from_x - (to_x + to_width));
                }
                // check for left wraparound
                else if (fabs((from_x + to_width) - to_x) < distx) {
                        distx = fabs((from_x + to_width) - to_x);
                }
                disty = fabs (from_y - to_y);

                // check for bottom wraparound
                if (fabs (from_y - (to_y + to_height)) < disty) {
                        disty = fabs (from_y - (to_y + to_height));
                }
                // check for top wraparound
                else if (fabs((from_y + to_height) - to_y) < disty) {
                        disty = fabs((from_y + to_height) - to_y);
                }

                if ((distx > (double)(proj_height)+0.5+ZERO) && (distx < (double)(proj_width)+0.5+ZERO) &&
                    (disty < (double)(proj_height)+0.5+ZERO)) {
                        synapse = true;
                }
        }
        else if (proj_type.compare ("vertiu" , Qt::CaseInsensitive) == 0) {

                distx = fabs (from_x - to_x);

                // check for right wraparound
                if (fabs (from_x - (to_x + to_width)) < distx) {
                        distx = fabs (from_x - (to_x + to_width));
                }
                // check for left wraparound
                else if (fabs((from_x + to_width) - to_x) < distx) {
                        distx = fabs((from_x + to_width) - to_x);
                }
                disty = fabs (from_y - to_y);

                // check for bottom wraparound
                if (fabs (from_y - (to_y + to_height)) < disty) {
                        disty = fabs (from_y - (to_y + to_height));
                }
                // check for top wraparound
                else if (fabs((from_y + to_height) - to_y) < disty) {
                        disty = fabs((from_y + to_height) - to_y);
                }

                if ((disty > (double)(proj_height)+0.5+ZERO) && (disty < (double)(proj_width)+0.5+ZERO) &&
                    (distx < (double)(proj_height)+0.5+ZERO)) {
                        synapse = true;
                }
        }
        // left uniform
        else if (proj_type.compare ("leftu" , Qt::CaseInsensitive) == 0) {

                distx = fabs (from_x - to_x);

                // check for right wraparound
                if (fabs (from_x - (to_x + to_width)) < distx) {
                        distx = fabs (from_x - (to_x + to_width));
                }
                // check for left wraparound
                else if (fabs((from_x + to_width) - to_x) < distx) {
                        distx = fabs((from_x + to_width) - to_x);
                }
                disty = fabs (from_y - to_y);

                // check for bottom wraparound
                if (fabs (from_y - (to_y + to_height)) < disty) {
                        disty = fabs (from_y - (to_y + to_height));
                }
                // check for top wraparound
                else if (fabs((from_y + to_height) - to_y) < disty) {
                        disty = fabs((from_y + to_height) - to_y);
                }

                if ((distx < (double)(proj_width)+0.5+ZERO) && (disty < (double)(proj_height)+0.5+ZERO) &&
                    (to_y < to_height/2)) {
                        synapse = true;
                }
        }
        // right uniform
        else if (proj_type.compare ("rightu" , Qt::CaseInsensitive) == 0) {

                distx = fabs (from_x - to_x);

                // check for right wraparound
                if (fabs (from_x - (to_x + to_width)) < distx) {
                        distx = fabs (from_x - (to_x + to_width));
                }
                // check for left wraparound
                else if (fabs((from_x + to_width) - to_x) < distx) {
                        distx = fabs((from_x + to_width) - to_x);
                }
                disty = fabs (from_y - to_y);

                // check for bottom wraparound
                if (fabs (from_y - (to_y + to_height)) < disty) {
                        disty = fabs (from_y - (to_y + to_height));
                }
                // check for top wraparound
                else if (fabs((from_y + to_height) - to_y) < disty) {
                        disty = fabs((from_y + to_height) - to_y);
                }

                if ((distx < (double)(proj_width)+0.5+ZERO) && (disty < (double)(proj_height)+0.5+ZERO) &&
                    (to_y >= to_height/2)) {
                        synapse = true;
                }
        }
        // circular uniform projection
        else if (proj_type.compare ("centeru" , Qt::CaseInsensitive) == 0) {

                dist = sqrt (square (from_x - to_x) + square (from_y - to_y));

                // check for diagonal wrap
                if (sqrt (square (from_x - (to_x + to_width)) + square (from_y - (to_y + to_height))) < dist) {
                        dist = sqrt (square (from_x - (to_x + to_width)) + square (from_y - (to_y+to_height)));
                        //fprintf(stderr,"diagonal wraparound!");
                }
                if (sqrt (square (from_x - (to_x + to_width)) + square (from_y - (to_y - to_height))) < dist) {
                        dist = sqrt (square (from_x - (to_x + to_width)) + square (from_y - (to_y-to_height)));
                }
                if (sqrt (square (from_x - (to_x - to_width)) + square (from_y - (to_y + to_height))) < dist) {
                        dist = sqrt (square (from_x - (to_x - to_width)) + square (from_y - (to_y+to_height)));
                }
                if (sqrt (square (from_x - (to_x - to_width)) + square (from_y - (to_y - to_height))) < dist) {
                        dist = sqrt (square (from_x - (to_x - to_width)) + square (from_y - (to_y-to_height)));
                }
                if (sqrt (square (from_x - (to_x + to_width)) + square (from_y - to_y)) < dist) {
                        dist = sqrt (square (from_x - (to_x + to_width)) + square (from_y - to_y));
                }
                // check for left wraparound
                if (sqrt (square ((from_x + to_width) - to_x) + square (from_y - to_y)) < dist) {
                        dist = sqrt (square ((from_x + to_width) - to_x) + square (from_y - to_y));
                }
                // check for top wraparound
                if (sqrt (square (from_x - to_x) + square (from_y - (to_y + to_height))) < dist) {
                        dist = sqrt (square (from_x - to_x) + square (from_y - (to_y + to_height)));
                }
                // check for bottom wraparound
                if (sqrt (square (from_x - to_x) + square ((from_y + to_height) - to_y)) < dist) {
                        dist = sqrt (square (from_x - to_x) + square ((from_y + to_height) - to_y));
                }
                // dist is now the min distance to any of the dual representations.



                if ((dist < (double)(proj_height)+ZERO)) {
                        synapse = true;
                }
        }
        // surround uniform projection
        else if (proj_type.compare ("surroundu" , Qt::CaseInsensitive) == 0)
        {
#ifdef PROJECTION_DEBUG
                        qDebug() << "Arbor: " << QString::fromStdString(proj_type) << endl;
#endif

#ifdef NEWER_STUFF
                dist = sqrt (square (from_x - to_x) + square (from_y - to_y));

                // check for diagonal wrap
                if (sqrt (square (from_x - (to_x + to_width)) + square (from_y - (to_y + to_height))) < dist) {
                        dist = sqrt (square (from_x - (to_x + to_width)) + square (from_y - (to_y+to_height)));
                        //fprintf(stderr,"diagonal wraparound!");
                }
                if (sqrt (square (from_x - (to_x + to_width)) + square (from_y - (to_y - to_height))) < dist) {
                        dist = sqrt (square (from_x - (to_x + to_width)) + square (from_y - (to_y-to_height)));
                }
                if (sqrt (square (from_x - (to_x - to_width)) + square (from_y - (to_y + to_height))) < dist) {
                        dist = sqrt (square (from_x - (to_x - to_width)) + square (from_y - (to_y+to_height)));
                }
                if (sqrt (square (from_x - (to_x - to_width)) + square (from_y - (to_y - to_height))) < dist) {
                        dist = sqrt (square (from_x - (to_x - to_width)) + square (from_y - (to_y-to_height)));
                }
                if (sqrt (square (from_x - (to_x + to_width)) + square (from_y - to_y)) < dist) {
                        dist = sqrt (square (from_x - (to_x + to_width)) + square (from_y - to_y));
                }
                // check for left wraparound
                if (sqrt (square ((from_x + to_width) - to_x) + square (from_y - to_y)) < dist) {
                        dist = sqrt (square ((from_x + to_width) - to_x) + square (from_y - to_y));
                }
                // check for top wraparound
                if (sqrt (square (from_x - to_x) + square (from_y - (to_y + to_height))) < dist) {
                        dist = sqrt (square (from_x - to_x) + square (from_y - (to_y + to_height)));
                }
                // check for bottom wraparound
                if (sqrt (square (from_x - to_x) + square ((from_y + to_height) - to_y)) < dist) {
                        dist = sqrt (square (from_x - to_x) + square ((from_y + to_height) - to_y));
                }
                // dist is now the min distance to any of the dual representations.

                if ((dist > (double)(proj_height)+0.5+ZERO) && (dist < (double)(proj_width)+0.5+ZERO)) {
                        synapse = true;
                }
#endif


                dist = sqrt (pow (from_x - to_x, 2.0) + pow (from_y - to_y, 2.0));

                // check for right wraparound
                if (sqrt (pow (from_x - (to_x + to_width),2.0) + pow (from_y - to_y, 2.0)) < dist) {
                        dist = sqrt (pow (from_x - (to_x + to_width),2.0) + pow (from_y - to_y, 2.0));
                }
                // check for left wraparound
                else if (sqrt (pow ((from_x + to_width) - to_x ,2.0) + pow (from_y - to_y, 2.0)) < dist) {
                        dist = sqrt (pow ((from_x + to_width) - to_x ,2.0) + pow (from_y - to_y, 2.0));
                }
                // check for top wraparound
                else if (sqrt (pow (from_x - to_x, 2.0) + pow (from_y - (to_y + to_height), 2.0)) < dist) {
                        dist = sqrt (pow (from_x - to_x, 2.0) + pow (from_y - (to_y + to_height), 2.0));
                }
                // check for bottom wraparound
                else if (sqrt (pow (from_x - to_x, 2.0) + pow ((from_y + to_height) - to_y, 2.0)) < dist) {
                        dist = sqrt (pow (from_x - to_x, 2.0) + pow ((from_y + to_height) - to_y, 2.0));
                }

                if ((dist > (double)(proj_height)+0.5+ZERO) && (dist < (double)(proj_width)+0.5+ZERO)) {
                        synapse = true;
                }
        }
        // surround uniform projection
        else if (proj_type.compare ("surroundu_no" , Qt::CaseInsensitive) == 0) {

                dist = sqrt (square (from_x - to_x) + square (from_y - to_y));

                if ((dist > (double)(proj_height)+0.5+ZERO) && (dist < (double)(proj_width)+0.5+ZERO)) {
                        synapse = true;
                }
        }
        // lateral uniform projection
        else if (proj_type.compare ("lateralu" , Qt::CaseInsensitive) == 0)
        {
                dist = fabs (from_x - to_x);

                // check for right wraparound
                if (fabs (from_x - (to_x + to_width)) < dist) {
                        dist = fabs (from_x - (to_x + to_width));
                }
                // check for left wraparound
                else if (fabs((from_x + to_width) - to_x) < dist) {
                        dist = fabs((from_x + to_width) - to_x);
                }

                if ((dist > (double)(proj_height)+0.5+ZERO) && (dist < (double)(proj_width)+0.5+ZERO)) {
                        synapse = true;
                }
        }
        // circular gaussian projection
        else if (proj_type.compare ("centerg" , Qt::CaseInsensitive) == 0)
        {
                dist = sqrt (square (from_x - to_x) + square (from_y - to_y));

                // check for right wraparound
                if (sqrt (square (from_x - (to_x + to_width)) + square (from_y - to_y)) < dist) {
                        dist = sqrt (square (from_x - (to_x + to_width)) + square (from_y - to_y));
                }
                // check for left wraparound
                else if (sqrt (square ((from_x + to_width) - to_x) + square (from_y - to_y)) < dist) {
                        dist = sqrt (square ((from_x + to_width) - to_x) + square (from_y - to_y));
                }
                // check for top wraparound
                else if (sqrt (square (from_x - to_x) + square (from_y - (to_y + to_height))) < dist) {
                        dist = sqrt (square (from_x - to_x) + square (from_y - (to_y + to_height)));
                }
                // check for bottom wraparound
                else if (sqrt (square (from_x - to_x) + square ((from_y + to_height) - to_y)) < dist) {
                        dist = sqrt (square (from_x - to_x) + square ((from_y + to_height) - to_y));
                }

                if ((rnd01 () < exp (-(dist/square ((double)proj_height))))) {
                        synapse = true;
                }
        }
        // surround gaussian projection
        else if (proj_type.compare("surroundg" , Qt::CaseInsensitive) == 0)
        {
                dist = sqrt (square (from_x - to_x) + square (from_y - to_y));

                // check for right wraparound
                if (sqrt (square (from_x - (to_x + to_width)) + square (from_y - to_y)) < dist) {
                        dist = sqrt (square (from_x - (to_x + to_width)) + square (from_y - to_y));
                }
                // check for left wraparound
                else if (sqrt (square ((from_x + to_width) - to_x) + square (from_y - to_y)) < dist) {
                        dist = sqrt (square ((from_x + to_width) - to_x) + square (from_y - to_y));
                }
                // check for top wraparound
                else if (sqrt (square (from_x - to_x) + square (from_y - (to_y + to_height))) < dist) {
                        dist = sqrt (square (from_x - to_x) + square (from_y - (to_y + to_height)));
                }
                // check for bottom wraparound
                else if (sqrt (square (from_x - to_x) + square ((from_y + to_height) - to_y)) < dist) {
                        dist = sqrt (square (from_x - to_x) + square ((from_y + to_height) - to_y));
                }

                r = rnd01 ();
                if ((r > exp (-(dist/square ((double)proj_height)))) && (r < exp (-(dist/square ((double)proj_width))))) {
                        synapse = true;
                }
        }
        // From Left Side
        else if (proj_type.compare ("source_block_nontopo" , Qt::CaseInsensitive) == 0)
        {
                if (((from_inx%from_width) >= proj_height) && ((from_inx%from_width) <= proj_width)) {
                        synapse = true;
                }
        }
        // One to one match synapse
        else if (proj_type.compare ("match" , Qt::CaseInsensitive) == 0)
        {
                if (from_inx == to_inx)
                {
                        synapse = true;
                }
        }
        else
        { //* Need to handle this in a better way!
                qDebug() << "Invalid projection type: " << proj_type << endl;
                //exit (1);
        }
        return synapse;

} // end Network_Synapse

Group* Network::GetGrp(QString GrpName)
{
        for (list<Group*>::iterator it = Groups.begin();
                        it != Groups.end(); ++it)
        {
                if (GrpName.toLower().compare(QString::fromStdString((*it)->GetName()).toLower()) == 0) {
                        return *it;
                }
                else{
                        continue;
                }
        }
        return NULL;
}

void Network::Cxn(QStringList Params)
{
        bool ConnectionMade = false;
        MicroCxn* NewMicro;
        MacroCxn* NewMacro;

        //could allocate chunks of memory here, i.e.
        //MicroCxn* NewMicroCxns = new MicroCxn[1000];
        //MicroCxn* NewMicros;
        /*MicroCxn TestMySize;
        Coeff CoeffSize;
        qDebug() << "Size of: " << sizeof(TestMySize) << endl;
        qDebug() << "Size of 2: " << sizeof(CoeffSize) << endl;*/

        NewMacro = new MacroCxn(Params);
        MacroCxns.push_back(NewMacro);
        ++nct;       /* Increment number of Conntypes in system */

        OutputWin::message("Initializing Connection: " + QString::fromStdString(NewMacro->GetPreGrp()->GetName()) + " to "
                + QString::fromStdString(NewMacro->GetPostGrp()->GetName()));

        /* How about special match routine for speed?*/
        /* Debug: is it an allocation problem? */
        //++SpecialCounter;
        //qDebug() << "Special Counter: " << SpecialCounter << endl;

        //*?* Need to check if group exists first before going forward!

        //*?* I need to output a warning if the QtyInCxns > MAX_SYNAPSES_LIMIT
        // count the number of connections for allocation.
        for (int i = 0; i < NewMacro->GetPreGrp()->GetSize(); i++)
        {
                for (int j = 0; j < NewMacro->GetPostGrp()->GetSize(); j++)
                {
                        ConnectionMade = !((NewMacro->GetPreGrp() == NewMacro->GetPostGrp()) && (i == j)) &&
                                (NewMacro->GetPostGrp()->GetCell(j)->GetNumInCxns() < MAX_SYNAPSES_PER_CELL_LIMIT)  &&
                                Network::Synapse(NewMacro->GetHeight(), NewMacro->GetWidth(), NewMacro->GetXoff(),
                                                                  NewMacro->GetYoff(), NewMacro->GetAng(), NewMacro->GetPreGrp()->GetHeight(),                                                                                                                            NewMacro->GetPreGrp()->GetWidth(), NewMacro->GetPostGrp()->GetHeight(),
                                                                  NewMacro->GetPostGrp()->GetWidth(), i, j, NewMacro->GetArbor(), (double)NewMacro->GetProb());
                        if (ConnectionMade)
                        {
                                //if (SpecialCounter != 7)
                                //{
                                        //remove for speed *? -- how can I pull this new memory allocation out of this loop?
                                        NewMicro = new MicroCxn(NewMacro->GetPreGrp()->GetCell(i), NewMacro->GetPostGrp()->GetCell(j), NewMacro);
                                        //qDebug() << "Connection made!" << endl;

                                        NewMacro->AddMicro(NewMicro);
                                        ++TotCxns;
                                //}
                        } // if Network_Synapse
                } // end for j
        } // end for i
}// end Network_Connection

//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
//@                                                                          @
//@  Title: void Network_Group (char *line)                                  @
//@                                                                          @
//@  Description: Takes a group line from the parameter file and sets        @
//@               up the group parameters and constants.                     @
//@                                                                          @
//@  Input:  line - a group line from the parameter file.                    @
//@  Output: none                                                            @
//@                                                                          @
//@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

Group* Network::Grp(QStringList Params) {

   int GrpID = (int)Groups.size();
   Group* NewGroup;
   Cell* cell;

   try {
      NewGroup = new Group(Params, GrpID);
      }
   catch (exception& e) {
      qDebug() << "In module Network::Grp() and Exception has occurred: " << e.what() << endl;
      }

   //initialize cells in group -- dynamically?
   for (int i = 0; i < NewGroup->GetSize(); ++i) {
      try {
         cell = new Cell(i, NewGroup);
         }
      catch (exception& e) {
         qDebug() << "In module Network::Grp() and Exception has occurred: " << e.what() << endl;
         }
      NewGroup->AddCell(cell);
      }
   //NewGroup->SortCells();
   Groups.push_back(NewGroup);

   return NewGroup;
   } // end Network_Group

/*---------------------------------------------------------------------*
*  Routines to check values of tokens (int or double)                  *
*  Added, 11/25/09, GNR to reduce the relentless repetitiveness        *
*     of the original code in Network::InspectLine                     *
*---------------------------------------------------------------------*/

bool Network::CheckIntTok(QString TokName, QStringList Tokens,
      int line, int itok, int low, int high) {

   int ival = Tokens[itok].toInt();
   if (ival < low || ival > high) {
      OutputWin::error(0, "( line " + QString::number(line) +
         ", arg " + QString::number(itok) +
         ", input value " + Tokens[itok] + "): " +
         TokName + " must be within range from " +
         QString::number(low) + " to " + QString::number(high));
      return false;
      }
   return true;
   } /* End CheckIntTok() */

bool Network::CheckDblTok(QString TokName, QStringList Tokens,
      int line, int itok, double dlow, double dhigh) {

   double val = Tokens[itok].toDouble();
   if (val <= dlow || ival >= dhigh) {
      OutputWin::error(0, "( line " + QString::number(line) +
         ", arg " + QString::number(itok) +
         ", input value " + Tokens[itok] + "): " +
         TokName + " must be within range from " +
         QString::number(dlow) + " to " + QString::number(dhigh));
      return false;
      }
   return true;
   } /* End CheckDblTok() */



/*---------------------------------------------------------------------*
*                        Network::InspectLine                          *
*---------------------------------------------------------------------*/

bool Network::InspectLine(QString FileName, QStringList Tokens,
      int lineNum, QStringList GrpCheck) {

   double tval;

   /* These Limitations & Values must stay in sync
   *  w/ special changes i.e. Init Connection/fixed pt variations.
   *  NOTE: will need updates to accomodate Darwin 7 limits etc.
   *  Rev, 11/10/09, GNR - Modify tests for Izhikevich test run
   *  Rev, 11/25/09, GNR - Add Check routines,
   *     modify for RockVis group type
   */

   /***** Group Limits *****/
   int GrpToks = 9; //including 'g'
   int Grp5Toks = 17;
   int GrpTLim[2] = { 0, 20 };
   int GrpHLim[2] = { 1, 65536 };
   int GrpWLim[2] = { 1, 65536 };
   // Fixed Pt. Values
   double SclLim[2] ee= { 0, 1.0 };
   double FireLim[2] = { 0, 1.0 };
   double VDLim[2] = { 0, 1.0 };
   double PrstLim[2] = { 0, 1.0 };

   /***** Connection Limits *****/
   int CxnToks = 25; //including 'c'
   int CxnToksNoPlas = 16;
   int CxnTLim[2] = { 0, 0x09 };
   int CxnHLim[2] = { 0, 10000 };
   int CxnWLim[2] = { 0, 10000 };
   QStringList Arbors;
   Arbors << "nontopo" << "blocku" << "blocku_no" << "blocku_no_left"
      << "blocku_no_right" << "blocku_no_up" << "blocku_no_down"
      << "blocku_no_dl" << "blocku_no_dr" << "blocku_no_ul"
      << "blocku_no_ur" << "diaguR_no" << "diaguL_no" << "blocku_no"
      << "leftonlyfrm" << "rightonlyfrm" << "leftonlyto"
      << "rightonlyto" << "horizu" << "vertiu" << "leftu"
      << "rightu" << "centeru" << "surroundu" << "surroundu_no"
      << "lateralu" << "centerg" << "surroundg"
      << "source_block_nontopo" << "match";
   // val grp, exists && val-dep!
   double ProbLim[2] = { 0.0, 1.0 };
   int XoffLim[2] = { 0, 10000 };
   int YoffLim[2] = { 0, 10000 };
   int AngLim[2] = { 0, 10000 };
   int PlastTLim[2] = { 0, 6 };
   int IILim[2] = { 0, 32767 };
   int IRLim[2] = { 0, 32767 };
   int ISigLim[2] = { 0, 32767 };
   //Fixed Point Values
   double wLim[2] = { -32.0, 32.0 };//weights
   double InflLim[2] = { -1.0, 1.0 };
   double PlastMaxLim[2] = { -32.0, 32.0 };
   double LRLim[2] = { -1.0, 1.0 };
   double EMinLim[2] = { 0.0, 1.0 };
   double ERLim[2] = { 0.0, 1.0 };
   double InSigLim[2] = { 0.0, 1.0 };

   if (Tokens[0].toLower() == "g") {    // group line

      int igrp = Tokens[2].toInt();
      int Numtoks = (igrp == 5) ? Grp5Toks : GrpToks;

      if (Tokens.size() != Numtoks) {
         OutputWin::error(0, FileName + "( ln " +
            QString::number(lineNum) + " ): " +
            "Group definitions take " + QString::number(Numtoks-1) +
            " argument(s)");
         return false;
         }
      if (!CheckIntTok(QString("Group type"), Tokens, lineNum, 2,
         GrpTLim[0], GrpTLim[1])) return false;
      if (!CheckIntTok(QString("Group height"), Tokens, lineNum, 3,
         GrpHLim[0], GrpHLim[1])) return false;
      if (!CheckIntTok(QString("Group width"), Tokens, lineNum, 4,
         GrpWLim[0], GrpWLim[1])) return false;
      if (igrp == GT_Izhik) {
         /* N.B.  Range limits on Izhikevich parameters take into
         *  account fact that k, b, d will be divided by Cm  */
         if (!CheckDblTok(QString("Izhikevich a"), Tokens, lineNum, 5,
            -2.0, 2.0)) return false;
         if (!CheckDblTok(QString("Izhikevich b"), Tokens, lineNum, 6,
            -32.0, 32.0)) return false;
         if (!CheckDblTok(QString("Izhikevich c"), Tokens, lineNum, 7,
            -128.0, 128.0)) return false;
         if (!CheckDblTok(QString("Izhikevich d"), Tokens, lineNum, 8,
            -512.0, 512.0)) return false;
         if (!CheckDblTok(QString("Izhikevich k"), Tokens, lineNum, 9,
            0.0, 8.0)) return false;
         if (!CheckDblTok(QString("Izhikevich vT"), Tokens, lineNum,
            10, -128.0, 128.0)) return false;
         if (!CheckDblTok(QString("Izhikevich vPeak"), Tokens, lineNum,
            11, -128.0, 128.0)) return false;
         if (!CheckDblTok(QString("Cm"), Tokens, linenum,
            12, 0.0, 256.0)) return false;
         if (!CheckDblTok(QString("Izhi var a"), Tokens, lineNum,
            13, -2.0, 2.0)) return false;
         if (!CheckDblTok(QString("Izhi var b"), Tokens, lineNum,
            14, -32.0, 32.0)) return false;
         if (!CheckDblTok(QString("Izhi var c"), Tokens, lineNum,
            15, -128.0, 128.0)) return false;
         if (!CheckDblTok(QString("Izhi var d"), Tokens, lineNum,
            16, -512.0, 512.0)) return false;
         } /* End checking Izhikevich tokens */

      else {                    /* Original (non-Izhikevich) group types here */
         if (Tokens[5].toDouble() < SclLim[0] || Tokens[5].toDouble() > SclLim[1]) {
            OutputWin::warning(0, FileName + "( ln " + QString::number(lineNum) + ", arg 5 ): "
               + "Group scale must be within range from " + QString::number(SclLim[0]) + " to "
               + QString::number(SclLim[1]) + ". Value will be normalized up/down to limit.");
            }
         if (Tokens[6].toDouble() < FireLim[0] || Tokens[6].toDouble() > FireLim[1]) {
            OutputWin::warning(0, FileName + "( ln " + QString::number(lineNum) + ", arg 6 ): "
               + "Group Firing Threshold must be within range from " + QString::number(FireLim[0]) + " to "
               + QString::number(FireLim[1]) + ". Value will be normalized up/down to limit.");
            }
         if (Tokens[7].toDouble() < VDLim[0] || Tokens[7].toDouble() > VDLim[1]) {
            OutputWin::warning(0, FileName + "( ln " + QString::number(lineNum) + ", arg 7 ): "
               + "Group Voltage-Dependent Threshold must be within range from " + QString::number(VDLim[0]) + " to "
               + QString::number(VDLim[1]) + ". Value will be normalized up/down to limit.");
            }
         if (Tokens[8].toDouble() < PrstLim[0] || Tokens[8].toDouble() > PrstLim[1]) {
            OutputWin::warning(0, FileName + "( ln " + QString::number(lineNum) + ", arg 8 ): "
               + "Group Persistence must be within range from " + QString::number(PrstLim[0]) + " to "
               + QString::number(PrstLim[1]) + ". Value will be normalized up/down to limit.");
            }
         return true;
         }
      } // end group line

   else if (Tokens[0].toLower() == "c") {

      int iplas = Tokens[15].toInt();
      if ((iplas == 0 && Tokens.size() < CxnToksNoPlas) || (
            iplas != 0 && Tokens.size() != CxnToks)) {
         OutputWin::error(0, FileName + "( ln " + QString::number(lineNum) + " ): "
            + "Connection definitions take " + QString::number(CxnToks-1) + " argument(s)");
         return false;
         }
      if (!GrpCheck.contains(Tokens[1], Qt::CaseInsensitive)) {
         OutputWin::error(0, FileName + "( ln " + QString::number(lineNum) + ", arg 1 ): "
            + "Pre-Synaptic Group \"" + Tokens[1] + "\" does not exist");
         return false;
         }
      if (!GrpCheck.contains(Tokens[2], Qt::CaseInsensitive)) {
         OutputWin::error(0, FileName + "( ln " + QString::number(lineNum) + ", arg 2 ): "
            + "Post-Synaptic Group \"" + Tokens[2] + "\" does not exist");
         return false;
         }
      if (Tokens[4].toInt(0,16) < CxnTLim[0] || Tokens[4].toInt(0,16) > CxnTLim[1]) {
         OutputWin::error(0, FileName + "( ln " + QString::number(lineNum) + ", arg 4 ): "
            + "Connection types must be within range " + QString::number(CxnTLim[0]) + " to "
            + QString::number(CxnTLim[1])) ;
         return false;
         }
      if (Tokens[5].toInt() < CxnHLim[0] || Tokens[5].toInt() > CxnHLim[1]) {
         OutputWin::error(0, FileName + "( ln " + QString::number(lineNum) + ", arg 5 ): "
            + "Connection height must be within range " + QString::number(CxnHLim[0]) + " to "
            + QString::number(CxnHLim[1])) ;
         return false;
         }
      if (Tokens[6].toInt() < CxnWLim[0] || Tokens[6].toInt() > CxnWLim[1]) {
         OutputWin::error(0, FileName + "( ln " + QString::number(lineNum) + ", arg 6 ): "
            + "Connection height must be within range " + QString::number(CxnWLim[0]) + " to "
            + QString::number(CxnWLim[1])) ;
         return false;
         }
      if (!Arbors.contains(Tokens[7],  Qt::CaseInsensitive)) {
         OutputWin::error(0, FileName + "( ln " + QString::number(lineNum) + ", arg 7 ): "
            + "Arbor definition \"" + Tokens[7] + "\" does not exist") ;
         return false;
         }
      if (Tokens[8].toDouble() < ProbLim[0] || Tokens[8].toDouble() > ProbLim[1] {
         OutputWin::error(0, FileName + "( ln " + QString::number(lineNum) + ", arg 8 ): "
            + "Probability values must be within range " + QString::number(ProbLim[0]) + " to "
            + QString::number(ProbLim[1])) ;
         return false;
         }
      if (Tokens[9].toInt() < XoffLim[0] || Tokens[9].toInt() > XoffLim[1]) {
         OutputWin::error(0, FileName + "( ln " + QString::number(lineNum) + ", arg 9 ): "
            + "Xoff must be within range " + QString::number(XoffLim[0]) + " to "
            + QString::number(XoffLim[1])) ;
         return false;
         }
      if (Tokens[10].toInt() < YoffLim[0] || Tokens[10].toInt() > YoffLim[1]) {
         OutputWin::error(0, FileName + "( ln " + QString::number(lineNum) + ", arg 10 ): "
            + "Yoff must be within range " + QString::number(YoffLim[0]) + " to "
            + QString::number(YoffLim[1])) ;
         return false;
         }
      if (Tokens[11].toInt() < AngLim[0] || Tokens[11].toInt() > AngLim[1]) {
         OutputWin::error(0, FileName + "( ln " + QString::number(lineNum) + ", arg 11 ): "
            + "Angle must be within range " + QString::number(AngLim[0]) + " to "
            + QString::number(AngLim[1])) ;
         return false;
         }
      if (Tokens[12].toDouble() > Tokens[13].toDouble()) {
         OutputWin::error(0, FileName + "( ln " + QString::number(lineNum) + ", arg 12 ): "
            + "Wmin must be less than or equal to Wmax");
         return false;
         }
      if (Tokens[12].toDouble() < wLim[0] || Tokens[12].toDouble() > wLim[1]) {
         OutputWin::error(0, FileName + "( ln " + QString::number(lineNum) + ", arg 12 ): "
            + "Wmin must be within range " + QString::number(wLim[0]) + " to "
            + QString::number(wLim[1])) ;
         return false;
         }
      if (Tokens[13].toDouble() < wLim[0] || Tokens[13].toDouble() > wLim[1]) {
         OutputWin::error(0, FileName + "( ln " + QString::number(lineNum) + ", arg 13 ): "
            + "Wmax must be within range " + QString::number(wLim[0]) + " to "
            + QString::number(wLim[1])) ;
         return false;
         }
      if (iplas < PlastTLim[0] || iplas > PlastTLim[1]) {
         OutputWin::error(0, FileName + "( ln " + QString::number(lineNum) + ", arg 15 ): "
            + "Plastic Connection types must be within range " + QString::number(PlastTLim[0]) + " to "
            + QString::number(PlastTLim[1])) ;
         return false;
         }
      if (iplas && (!GrpCheck.contains(Tokens[24], Qt::CaseInsensitive) &&
            Tokens[24].toLower() != "none")) {
         OutputWin::error(0, FileName + "( ln " + QString::number(lineNum) + ", arg 24 ): "
            + "Value Group \"" + Tokens[24] + "\" does not exist");
         return false;
         }

      // Warnings
      if (Tokens[14].toDouble() < InflLim[0] || Tokens[14].toDouble() > InflLim[1]) {
         OutputWin::warning(0, FileName + "( ln " + QString::number(lineNum) + ", arg 14 ): "
            + "Influence must be within range from " + QString::number(InflLim[0]) + " to "
            + QString::number(InflLim[1]) + ". Value will be normalized up/down to limit.");
         }

      // Remaining tests only apply if connection type is plastic
      if (!iplas) return true;

      if ((Tokens[16].toDouble() < PlastMaxLim[0] || Tokens[16].toDouble() > PlastMaxLim[1])) {
         OutputWin::warning(0, FileName + "( ln " + QString::number(lineNum) + ", arg 16 ): "
            + "Maximum plastic weight must be within range from " + QString::number(PlastMaxLim[0]) + " to "
            + QString::number(PlastMaxLim[1]) + ". Value will be normalized up/down to limit.");
         }
      if ((Tokens[17].toDouble() < LRLim[0] || Tokens[17].toDouble() > LRLim[1])) {
         OutputWin::warning(0, FileName + "( ln " + QString::number(lineNum) + ", arg 17 ): "
            + "Learning Rate must be within range from " + QString::number(LRLim[0]) + " to "
            + QString::number(LRLim[1]) + ". Value will be normalized up/down to limit.");
         }
      if ((Tokens[18].toInt() < IILim[0] || Tokens[18].toInt() > IILim[1])) {
         OutputWin::warning(0, FileName + "( ln " + QString::number(lineNum) + ", arg 18 ): "
            + "Internal Init must be within range from " + QString::number(IILim[0]) + " to "
            + QString::number(IILim[1]) + ". Value will be normalized up/down to limit.");
         }
      if ((Tokens[19].toInt() < IRLim[0] || Tokens[19].toInt() > IRLim[1])) {
         OutputWin::warning(0, FileName + "( ln " + QString::number(lineNum) + ", arg 19 ): "
            + "Internal Rate must be within range from " + QString::number(IRLim[0]) + " to "
            + QString::number(IRLim[1]) + ". Value will be normalized up/down to limit.");
            }
      if ((Tokens[20].toInt() < ISigLim[0] || Tokens[20].toInt() > ISigLim[1])) {
         OutputWin::warning(0, FileName + "( ln " + QString::number(lineNum) + ", arg 20 ): "
            + "Internal Threshold must be within range from " + QString::number(ISigLim[0]) + " to "
            + QString::number(ISigLim[1]) + ". Value will be normalized up/down to limit.");
         }
      if ((Tokens[21].toDouble() < EMinLim[0] || Tokens[21].toDouble() > EMinLim[1])) {
         OutputWin::warning(0, FileName + "( ln " + QString::number(lineNum) + ", arg 21 ): "
            + "Trace Eligibility Minimum must be within range from " + QString::number(EMinLim[0]) + " to "
            + QString::number(EMinLim[1]) + ". Value will be normalized up/down to limit.");
         }
      if ((Tokens[22].toDouble() < ERLim[0] || Tokens[22].toDouble() > ERLim[1])) {
         OutputWin::warning(0, FileName + "( ln " + QString::number(lineNum) + ", arg 22 ): "
            + "Trace Eligibility Rate must be within range from " + QString::number(ERLim[0]) + " to "
            + QString::number(ERLim[1]) + ". Value will be normalized up/down to limit.");
         }
      if ((Tokens[23].toDouble() < InSigLim[0] || Tokens[23].toDouble() > InSigLim[1])) {
         OutputWin::warning(0, FileName + "( ln " + QString::number(lineNum) + ", arg 23 ): "
            + "Input Threshold must be within range from " + QString::number(ERLim[0]) + " to "
            + QString::number(ERLim[1]) + ". Value will be normalized up/down to limit.");
         }

      return true;
      } // End c line

   else {                     /* Anything not g or c */
      return true;
      }

   } /* End InspectLine() */

void Network::ProcessFile(QFile* AnatomyFile) {

   QTextStream input(AnatomyFile);
   QStringList Tokens;
   int line = 1;
   bool InspectionFlag = false;
   QStringList GrpLst;

   /* Inspect each line for errors first */
   while (!input.atEnd()) {

      Tokens = input.readLine().split(QRegExp("\\s+"),
         QString::SkipEmptyParts);
      if (Tokens.size()) {
         if (Tokens[0].at(0) != '#' && (Tokens[0].toLower() == "c" ||
               Tokens[0].toLower() == "g")) {
            qDebug() << "Inspecting ( " + QString::number(line) +
               " ): "  << Tokens << endl;

#ifndef NPU_MEMORY_TESTCASE_NO_NETWORK_INSPECTION
            InspectionFlag = InspectLine(AnatomyFile->fileName(),
               Tokens, line, GrpLst);
            if (!InspectionFlag) {
               OutputWin::message("The Network Initialization process "
                  "has exited.");
               return;
               }
            else if (Tokens[0].toLower() == "g") {
               GrpLst.push_back(Tokens[1]);
               }
#else
            if (Tokens[0].toLower() == "g") {
               GrpLst.push_back(Tokens[1]);
               }
#endif
            }
         }
      ++line;
      }

   /* Process each line after no errors were found */
   input.seek(0); // return to beginning
   while (!input.atEnd()) = {
      Tokens = input.readLine().split(QRegExp("\\s+"),
         QString::SkipEmptyParts);
      if (Tokens.size()) {
         if (Tokens[0].toLower() == "g") {
            Network::Grp(Tokens);
            }
         else if (Tokens[0].toLower() == "c") {
            Network::Cxn(Tokens);
            }
         /* l = load weights from file in
         *  dir 'd' specified by nearest 'd'
         *  line using default file name*/
         else if (Tokens[0].toLower() == "l") {
            Network::Cxn(Tokens);
            }
         }
      }
   OutputWin::message("Network Initialization Complete!");;
   }
