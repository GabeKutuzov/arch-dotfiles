/**
 * NeuroAnatomy Synthesizer v1.4
 *
 * The NeuroAnatomy Synthesizer
 * 1) Inputs a NeuroAnatomy file and
 * initializes the defined neural network
 * 2) Places neuronal groups on SPP2 chips to
 * be simulated
 * 3) Initializes coefficient memory utilized in
 * the SPP2 simulation.
 * 4) Creates files that implicitly places groups &
 * initializes memory.
 *
 * @Author: Richard G. Martin <RichardGMartin@gmail.com>
 * @Company: Roceres, Corporation
 *
 * Date: April 21, 2008
 *
 */

#include <QMainWindow>
#include <ctime>

#include "AdvWin.h"
#include "RndNums.h"
#include "SPP2Globals.h"
#include "MyApplication.h"

int main(int argc, char **argv)
{
        MyApplication app(argc, argv);

#ifdef BENCHMARK_SEED
                rndseed(1);
#else if
                rndseed(time(0));
#endif

        AdvWin* win = new AdvWin();
        win->show();

        return app.exec();
}