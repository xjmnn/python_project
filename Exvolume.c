/*-------------------------------------Extern volume control -----------------------------*/
#include "includes.h"

unsigned char ucVolSentByte( unsigned char ucByteData )
{
    unsigned char i, no_ack = 0;
    _CLI();
    for(i=0; i<8; i++)
    {
        if (ucByteData&0x80)
        {SET_VOL_DATA();}     //  SDA  = 1
        else
            CLR_VOL_DATA();     //  SDA = 0

       ucByteData <<= 1;        //Shift the value
       VOLII2CWAIT();
       SET_VOL_CLK();;          //clk = 1;
       VOLII2CWAIT();
       CLR_VOL_CLK();         //clk = 0;
       VOLII2CWAIT();
    }
    SET_VOL_DATA();      //SDA = 1;
    VOLII2CWAIT();
    SET_VOL_CLK();      //clk = 1;

    if ( bit_is_set(IIC_VOLDATA_REG,IIC_VOL_DATA ))
        no_ack = 1; //check the SDA=1;
    VOLII2CWAIT();
    CLR_VOL_CLK();         //clk = 0;
    VOLII2CWAIT();
    _SEI();
    return  no_ack;
}

// slave transfer data to master
unsigned char ucVolReceiveByte(void)
{
    unsigned char i;
    unsigned char bytedata=0;            /*init the write signal*/

//    CLR_IIC_DIR();          //Clear iic data pin AS input
    // Receive byte (MSB first)
    _CLI();
    SET_VOL_DATA();             //SDA = 1;
    for(i=0; i<8; i++)          /*need to cycle 8 times*/
    {
        VOLII2CWAIT();
        CLR_VOL_CLK();         //clk = 0;
        VOLII2CWAIT();
        SET_VOL_CLK();;         //clk = 1;
//        II2CWAIT();
        bytedata <<= 1;
        if ( bit_is_set(IIC_VOLDATA_REG,IIC_VOL_DATA ))
            bytedata |= 0x01;

//        SET_IIC_DIR();
    }
    CLR_VOL_CLK();         //clk = 0;
    VOLII2CWAIT();
    _SEI();
    return bytedata;
}

/*****************************/
//Master send acknowledge bit to slave
//acknowledge="0",non-acknowledge="1"
void VolSendAcknowledge( unsigned char ack)
{
    if ( ack)
        {SET_VOL_DATA();}       //  SDA  = 1
    else
        CLR_VOL_DATA();         //  SDA = 0
    VOLII2CWAIT();
    SET_VOL_CLK();;         //clk = 1;
    VOLII2CWAIT();
    CLR_VOL_CLK();         //clk = 0;
    VOLII2CWAIT();
}

/*****************************/
//do 1 times,and un-check acknowledge
void VolByteWrite(unsigned char device, unsigned char address, unsigned char bytedata)
{
    IIC_VOL_START();
    ucVolSentByte(device);
    ucVolSentByte(address);
    ucVolSentByte(bytedata);
    IIC_VOL_STOP();
    VUmelay(10);
}

//retry,until acknowledge ok="0",if device bad,then do loop
void VolByteWrite1(unsigned char device,unsigned char address,unsigned char bytedata)
{
    unsigned char ack;
    IIC_VOL_START();
RESENT1:
    ack = ucVolSentByte(device);
    if ( ack==1 ) goto  RESENT1;  //acknowledge error

RESENT2:
    ucVolSentByte(address);
    if (ack==1) goto  RESENT2;

RESENT3:
    ucVolSentByte(bytedata);
    if (ack==1) goto  RESENT3;

    IIC_VOL_STOP();
    VUmelay(10);
}

void VolByteWrite2( unsigned char device,unsigned char address, unsigned char bytedata )
{
    unsigned char i;
    unsigned char ack;

    for( i=0; i<10; i++)         //time out,retry=10
    {
        IIC_VOL_START();
        ack=ucVolSentByte(device);  //sent out a byte ,identify numeric of device
        if (ack==1)
        {
            IIC_VOL_STOP();
            continue;
        }

        ack=ucVolSentByte(address);
        if (ack==1)
        {
            IIC_VOL_STOP();
            continue;
        }

        ack=ucVolSentByte(bytedata);
        if(ack==1)
        {
            IIC_VOL_STOP();
            continue;
        }
            IIC_VOL_STOP();
       if ( ack==0 )  break;
    }
    VUmelay(10);
}

/*****************************/
unsigned char VolI2cByteRead( unsigned char device,unsigned char address )
{
    unsigned char bytedata;
    IIC_VOL_START();
    ucVolSentByte(device);
    ucVolSentByte(address);
    IIC_VOL_START();
    ucVolSentByte(device|0x01);
    bytedata=ucVolReceiveByte( );
//    SendAcknowledge( 1 );
    SEND_VOL_NOACK()
    IIC_VOL_STOP();
    return bytedata;
}

/*****************************/
//master transfer count data to slave
unsigned char VolSentData( unsigned char* ucPData,unsigned char bytecnt )
{
    unsigned char i;

    for(i=0; i<bytecnt; i++)
    {
       if ( ucVolSentByte ( ucPData[i] ) == FAILURE )
          return FAILURE;
    }
    return SUCCESS;
}

void VWriteVolume(unsigned char ucDevice,unsigned char* ucPData,unsigned char ucByteCnt)
{
    IIC_VOL_START();
    ucVolSentByte(ucDevice);
//    ucI2cSentByte(ucStraddr);
    VolSentData( ucPData,ucByteCnt );
    IIC_VOL_STOP();
    VUmelay(10);
}

void VSetupSingleVolume( unsigned char ucChannelIdex,unsigned char ucVolume )
{
    unsigned char ucVol[3];
    unsigned char ucLen;
    unsigned char ucCurrentVol;
    ucCurrentVol = ucVolume;
    switch (ucChannelIdex)
    {
        case LEFTCHANNEL:
        {
            ucVol[0] = ( LEFT_CH_10STP|(ucCurrentVol/10) );
            ucVol[1] = ( LEFT_CH_1STP|(ucCurrentVol%10) );
            ucVol[2] = 0;
            ucLen = 2;
            VWriteVolume(DEVICEADR,ucVol, ucLen );
        }
        break;
        case RIGHTCHANNEL:
        {
            ucVol[0] = ( RIGHT_CH_10STP|(ucCurrentVol/10) );
            ucVol[1] = ( RIGHT_CH_1STP|(ucCurrentVol%10) );
            ucLen = 2;
            ucVol[2] = 0;
            VWriteVolume(DEVICEADR,ucVol,ucLen );
        }
        break;
        default:
          break;
    }
}

void VSetupDoulbeVolume( unsigned char ucVolume )
{
    unsigned char ucVol[3];
    unsigned char ucLen;
    unsigned char ucCurrentVol;
    ucCurrentVol = ucVolume;
    ucVol[0] = ( TWO_CH_10STP|(ucCurrentVol/10) );
    ucVol[1] = ( TWO_CH_1STP|(ucCurrentVol%10) );
    ucVol[2] = 0;
    ucLen = 2;
    VWriteVolume(DEVICEADR,ucVol, ucLen );
}

void VVolumeOff( void )
{
    unsigned char ucVol[2];
    ucVol [0] = FUNCTION_OFF;
    ucVol [1] = 0;
    VWriteVolume(DEVICEADR,ucVol, 1 );
}

void VMUTEOff( unsigned char ucMute )
{
    unsigned char ucVol[2];
    if ( ucMute )       //Mute off
        ucVol [0] = TWO_CH_MUTE_ON;
    else
        ucVol [0] = TWO_CH_MUTE_OFF;
    ucVol [1] = 0;
    VWriteVolume(DEVICEADR,ucVol, 1 );
}
