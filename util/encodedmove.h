#ifndef ENCODEDMOVE_H
#define ENCODEDMOVE_H

//
// An EncodedMove represents a gameplay action (i.e. action taken by the tank)
//

#define MAX_BITFIELD_VALUE(nbits) ((1<<(nbits))-1)

typedef struct EncodedMove {
    void clear()
    {
        u.value = 0;
    }

    bool isEmpty() const
    {
        return u.value == 0;
    }

    bool equals( EncodedMove other ) const
    {
        return u.value == other.u.value;
    }

    union {
        struct {
            // either of these first two bits being set identifies this move record
            unsigned char adjacent    :1; // 1 indicates moving to an new square identified by the prior recorded angle
            unsigned char rotate      :1; // 1 indicates rotating to the encoded angle

            unsigned char encodedAngle:2; // A value between 0-3 which corresponds to an angle of 0/90/180/270 respectively
            unsigned char shotCount   :4; // A number of times to fire at this move point
        } move;
// this macro needs to be defined using the above shotCount field's bit count:
#define MAX_MOVE_SHOT_COUNT MAX_BITFIELD_VALUE(4)

        struct {
            unsigned char header   :2; // 0 identifies this continuation record
            unsigned char shotCount:6; // Additional shot count to accumulate with its preceeding move record
        } continuation;
// this macro needs to be defined using the above shotCount field's bit count:
#define MAX_CONTINUATION_SHOT_COUNT MAX_BITFIELD_VALUE(6)

        unsigned char value;
    } u;
} EncodedMove;

#endif // ENCODEDMOVE_H
