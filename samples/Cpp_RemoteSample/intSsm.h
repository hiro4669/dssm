#ifndef __INTSSM_H__
#define __INTSSM_H__

#define SNAME_INT "intSsm"
#define PNAME_DOUBLE "doubleProperty"
#define NAME_SIZE 20

typedef struct
{
	int num;
} intSsm_k;

typedef struct {
	double dnum;
} doubleProperty_p;

typedef struct {
	int num1;
	double num2;
} intSsm_p;

typedef struct {
    int num;
    double dnum;
    char name[NAME_SIZE];
} props_p;

#endif
