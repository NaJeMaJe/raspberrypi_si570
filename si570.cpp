#include "si570.h"

Si570::Si570(QObject *parent) : QObject(parent)
{
    fd = wiringPiI2CSetup(SI570_ADDR);
    qDebug() << "Init I2C SI570 result: " << fd;
    double rfreq;

    reset();
    read_registers();

    hs_div_initial    = get_hsdiv();
    n1_initial              = get_n1();
    rfreq_long_inital = get_rfreq_truncated();

    rfreq =
            ((registers[1] & 0x3F)<<4)|
            ((registers[2] & 0xF0)>>4);

    fxtal = ((double)SI570_START_FREQ *
                   hs_div_initial * n1_initial) /
            rfreq;
}

void Si570::set_frequency(double frequency)
{
    quint8 hs_div_values[6] = {11,9,7,6,5,4};

    quint8 i;

    quint16 div;
    quint16 div_min;
    quint16 div_max;
    quint8 valid_combo;

    quint8 hs_div;
    quint8 n1;
    double  n1_float;

    double ratio;
    quint32 rfreq_final;

    //get minimum and maximum clock dividers that work for new frequency
    div_min = qCeil(SI570_FDCO_MIN/frequency);
    div_max = qFloor(SI570_FDCO_MAX/frequency);

    //go through the dividers and find one that is
    //possible to make with HS_DIV and N1 combo
    valid_combo = 0;
    for (div = div_min ; div <= div_max ; div++){
        for (i=0 ; i<6 ; i++){
            hs_div = hs_div_values[i]; //get a valid HS_DIV value
            n1_float = (double)div/(double)hs_div; //calculate N1 value

            if ((n1_float - floor(n1_float)) == 0){ //is n1_float an integer?
                n1 = (quint8) n1_float;
                if ((n1 == 1) || ((n1&1)==0)){ //is it a valid N1 divider?
                    valid_combo = 1; //A good divider combo! Let's get out!
                }

            }
            if (valid_combo)
                break;
        }
        if (valid_combo)
            break;
    }
    if (valid_combo == 0) //We failed at finding valid dividers :(
        return;

    ratio  = frequency / SI570_START_FREQ;
    ratio *= (double)n1 /(double)n1_initial;
    ratio *= (double)hs_div/(double)hs_div_initial;

    rfreq_final = ratio * rfreq_long_inital;

    //set new values to the registers array
    set_rfreq_truncated(rfreq_final);
    set_n1(n1);
    set_hsdiv(hs_div);

    //write new values to Si570 ram over i2c
    write_registers();
}

void Si570::reset()
{
    qDebug() << "reset" << wiringPiI2CWriteReg8(fd, 135, 0x01);
}

void Si570::read_registers()
{
    quint8 i;
    for (i=0; i<6; i++)
        registers[i] = wiringPiI2CReadReg8(fd,7+i);
}

void Si570::write_registers()
{
    quint8 i;
    quint8 tmp_reg135;
    quint8 tmp_reg137;

    //get current values of registers
    tmp_reg135 = wiringPiI2CReadReg8(fd,135);
    tmp_reg137 = wiringPiI2CReadReg8(fd,137);

    //Freeze DCO
    tmp_reg137 |= (1<<4);
    qDebug() << "write" << wiringPiI2CWriteReg8(fd,137,tmp_reg137);

    //write new configuration
    for ( i=0 ; i<6 ; i++)
        qDebug() << "write" << wiringPiI2CWriteReg8(fd,7+i,registers[i]);

    //Unfreeze DCO
    tmp_reg137 &= ~(1<<4);
    qDebug() << "write" << wiringPiI2CWriteReg8(fd,137,tmp_reg137);

    //Set NewFreq bit
    tmp_reg135 |= (1<<6);
    qDebug() << "write" << wiringPiI2CWriteReg8(fd,135,tmp_reg135);
}

quint8 Si570::get_hsdiv()
{
    return (registers[0]>>5) + 4;
}

void Si570::set_hsdiv(quint8 hsdiv)
{
    //if (hsdiv == 8 || hsdiv == 10)
    //    return;
    hsdiv -= 4;
    registers[0] &= 0b00011111;
    registers[0] |= hsdiv<<5;
}

quint8 Si570::get_n1()
{
    quint8 n1;
    n1  = (registers[0] & 0b00011111)<<2;
    n1 |= (registers[1]>>6) & 0b11;

    if (n1 == 0)
        n1 = 1;
    else if ((n1&1) != 0)
        n1 += 1;

    return n1;
}

void Si570::set_n1(quint8 n1)
{
    if (n1 == 1)
        n1 = 0;
    else if ((n1&1) == 0)
        n1 -= 1;

    registers[0] &= 0b11100000;
    registers[1] &= 0b00111111;
    registers[0] |= ((n1>>2) & 0b00011111);
    registers[1] |= (n1<<6) & 0b11000000;
}

quint32 Si570::get_rfreq_truncated()
{
    quint32 retval;

    retval = registers[1] & 0x3F;
    retval = (retval<<8) +  (registers[2]);
    retval = (retval<<8) +  (registers[3]);
    retval = (retval<<8) +  (registers[4]);
    retval = (retval<<6) + ((registers[5])>>2);

    return retval;
}

void Si570::set_rfreq_truncated(quint32 rfreq)
{
    registers[1] &= 0b11000000;
    registers[1] |= rfreq>>30;
    registers[2]  = rfreq>>22;
    registers[3]  = rfreq>>14;
    registers[4]  = rfreq>>6;
    registers[5]  = rfreq<<2;
}
