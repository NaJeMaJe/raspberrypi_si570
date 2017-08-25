#ifndef SI570_H
#define SI570_H

#include <QtCore>
#include <wiringPiI2C.h>

#define SI570_ADDR 0x55        ///< Si570 i2c address in 8bit format
#define SI570_START_FREQ 56.32 ///< Si570 Startup frequency in MHz
#define SI570_FDCO_MIN 4850    ///< Min frequency (MHz) of reference clock
#define SI570_FDCO_MAX 5670    ///< Max frequency (MHz) of reference clock

/* SI570 registers:
 * 7:  HS_DIV[2:0], N1[6:2]
 * 8:  N1[1:0], RFREQ[37:32]
 * 9:  RFREQ[31:24]
 * 10: RFREQ[23:16]
 * 11: RFREQ[15:8]
 * 12: RFREQ[7:0]
 * 13-18: registers 7-11 repeated for 7ppm version
 * 135: RST_REG,NewFreq,Freeze M,Freeze VCADC,0,0,0,RECALL
 * 137: 0,0,0,FREEZE_DCO,0,0,0,0
 */

class Si570 : public QObject
{
    Q_OBJECT

public:
    explicit Si570(QObject *parent = nullptr);
    void set_frequency(double frequency); // MHz

private:
    void reset();
    void read_registers();
    void write_registers();
    quint8 get_hsdiv();
    void set_hsdiv(quint8 hsdiv);
    quint8 get_n1();
    void set_n1(quint8 n1);
    quint32 get_rfreq_truncated();
    void set_rfreq_truncated(quint32 rfreq);

    int fd;
    quint8  registers[6];      ///<Si570 register values
    double  fxtal;             ///<Si570 XTAL frequency
    quint8  n1_initial;        ///<N1 initial value
    quint8  hs_div_initial;    ///<HS_DIV initial value
    quint32 rfreq_long_inital; ///<RFREQ initial truncated 32bit value


};

#endif // SI570_H
