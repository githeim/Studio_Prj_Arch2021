#ifndef _JITTERMETER_H_
#define _JITTERMETER_H_ 

#include <string>
#include <vector>
int SetupJitterMeter(std::string strAddr);
int GetJitter(long double &ldJitterTime_ms);

int  StartUpdateJitter(std::string strAddr);
void StopUpdateJitter();
int GetCurrentJitter(long double& ldJitterTime_ms);
#endif /* ifndef _JITTERMETER_H_ */
