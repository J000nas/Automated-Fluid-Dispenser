//
// Created by Jonas Will on 13.08.25.
//

#ifndef ANALOGLIMIT_H
#define ANALOGLIMIT_H



class AnalogLimit {
public:
    AnalogLimit();

    void calibrate();

    int getValue(int position) const;
private:
    int limits[5];


};



#endif //ANALOGLIMIT_H
