typedef struct 
{
    int can_id; //CAN id in the request
    int payload[2]; // payload of the request to poll. Will be two bytes;
    char pollName[15]; // Name of what is being polled
} HSFramePoll;
 