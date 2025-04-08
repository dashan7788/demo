#include "iicthread.h"

IICThread::IICThread(QObject *parent)
    : QObject{parent}
{
    qDebug()<<"IICThread线程号"<<QThread::currentThreadId();
    isIICThreadFlag = false;

    isSFM3300Flag = false;
    pSFM3300 = new SFM3300(this);

    isO2Flag = false;
    pO2 = new O2(this);

    isSCD4XFlag = false;
    pSCD4X = new SCD4X(this);


}

// 设置线程开关标志位
void IICThread::setIICThreadFlag(bool flag)
{
    isIICThreadFlag = flag;
}

//获取每秒参数
void IICThread::getData(float *o2c ,float *co2c ,float *gasflow)
{
    QReadWriteLock lock;
    // 写操作：加写锁，其他线程不能读或写
    lock.lockForWrite();
    if(mO2CCount > 0)
    {
        *o2c = mO2CSum / mO2CCount;
        mO2CSum = 0;
        mO2CCount = 0;
    }
    else
       *o2c = 0;

    if(mCO2CCount > 0)
    {
        *co2c = mCO2CSum / mCO2CCount / 100.0;
        mCO2CSum = 0;
        mCO2CCount = 0;
    }
    else
       *co2c = 0;

    if(mGasFlowCount > 0)
    {
        *gasflow = mGasFlowSum / mGasFlowCount / 60.0;
        mGasFlowSum = 0;
        mGasFlowCount = 0;
    }
    else
       *gasflow = 0;

    qDebug() << *o2c << " --" << *co2c  << " --" << *gasflow;

    lock.unlock();
}


void IICThread::runIICThreadFun()// 线程函数
{
    qDebug()<<"runIICThreadFun线程号"<<QThread::currentThreadId();
    bool ret= false;

    ret = pO2->initO2();
    if( ret == false){
        qDebug()<<"pO2->initO2错误";
        isO2Flag = false;
    }

    else{
        qDebug()<<"pO2->initO2成功";
        isO2Flag = true;
    }

   ret = pSFM3300->initSFM3300();
   if( ret == false)// SFM3300初始化失败，不执行传感器线程
   {
       qDebug()<<"SFM3300->initSFM3300错误";
        isSFM3300Flag = false;
   }
   else
   {
       qDebug()<<"SFM3300->initSFM3300成功";
       isSFM3300Flag = true;
   }

   ret = pSCD4X->SCD4X_Init();
   if( ret == false){
       qDebug()<<"pSCD4X->SCD4X_Init错误";
       isSCD4XFlag = false;

   }

   else{
       qDebug()<<"pSCD4X->SCD4X_Init成功";
       isSCD4XFlag = true;
   }

    while(isIICThreadFlag)// 死循环
    {
        QThread::msleep(100);//
        //emit signalsIICThread();

        pO2->readSerialPortData(&mO2C);
        if(mO2C > 10 && mO2C < 21)
        {
            mO2CCount++;
            mO2CSum +=mO2C;
        }
        pSFM3300->readSFM3300(&mGasFlow);
        if(mGasFlow >=0 && mGasFlow < 250)
        {
            mGasFlowCount++;
            mGasFlowSum +=mGasFlow;
        }

        pSCD4X->SCD4X_ReadMeasurement(&mCO2C, &mTemperature, &mHumidity);
        if(mCO2C > 100 && mCO2C < 4000)
        {
            mCO2CCount++;
            mCO2CSum +=mCO2C;
        }

//        if(isSFM3300Flag)
//        {
//            int ret1 = pSFM3300->readSFM3300(&mGasFlowSum[mGasFlowCount%300]);
//            if(ret1 != -1)
//            {
//                qDebug() << "气体瞬时流量点" <<mGasFlowCount%300 << "= "<< mGasFlowSum[mGasFlowCount];
//                mGasFlowCount ++;
//                mGasFlow = 0;
//                if(mGasFlowCount < 300){
//                    for(int i = 0 ; i < mGasFlowCount ; i++){
//                        mGasFlow = mGasFlow + mGasFlowSum[i];
//                    }
//                    mGasFlow = mGasFlow / mGasFlowCount;
//                    qDebug() << "气体流量 = " << mGasFlow << "mGasFlowCount = " <<mGasFlowCount;
//                }
//                else{
//                    for(int i = 0 ; i < 300 ; i++){
//                        mGasFlow = mGasFlow + mGasFlowSum[i];
//                    }
//                    mGasFlow = mGasFlow/300 ;
//                    qDebug() << "气体流量 = " << mGasFlow << "mGasFlowCount = " <<mGasFlowCount;
//                }
//                if(mGasFlowCount >= 600){
//                    mGasFlowCount = 300 ;
//                }

//            }
//            else
//            {
//                qDebug() << "ERROR------------------------";
//            }

//        }


//        if(isSCD4XFlag)
//        {
//            ret1 = pSCD4X->SCD4X_ReadMeasurement(&mCO2C, &mTemperature, &mHumidity);
//            if (ret1 == false)
//            {
//                 qDebug()<<"pSCD4X错误数据";
//            }
//            else if (mCO2C == 0)
//            {
//                qDebug()<<"无效样本";
//            }
//            else
//            {
//                qDebug()<<"mCO2C="<<mCO2C;
//                qDebug()<<"mTemperature="<<mTemperature/1000;
//                qDebug()<<"mHumidity="<<mHumidity/1000;
//            }
//        }



        if(!isIICThreadFlag)
        {
            break;
        }
    }
}
