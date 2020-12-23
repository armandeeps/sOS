#ifndef EXCEPTION_HANDLER_H
#define EXCEPTION_HANDLER_H

#define EXCEPTION_RET 256

/* exception handlers */
extern void exception_DE();
extern void exception_DB();
extern void exception_NMI();
extern void exception_BP();
extern void exception_OF();
extern void exception_BR();
extern void exception_UD();
extern void exception_NM();
extern void exception_DF();
extern void exception_CS();
extern void exception_TS();
extern void exception_NP();
extern void exception_SS();
extern void exception_GP();
extern void exception_PF();
extern void exception_MF();
extern void exception_AC();
extern void exception_MC();
extern void exception_XF();

#endif
