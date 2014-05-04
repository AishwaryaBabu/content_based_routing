/*!
    \file newport2.h 
    \brief Edited newport.h to avoid acknowledgements
*/
#include "common.h"
#include <iostream>
using namespace std;

class mySendingPort2 :public SendingPort
{

public:

  inline void timerHandler(){ timer_.startTimer(0.0);}

};
