/*
  Don't try to make sense of (or compile!) this code, it doesn't
  make sense. It's a variety of pasted C code that I've found to
  push the boundaries of the syntax coloring
*/

#undef	  EXTERN
#define	  EXTERN

#include "gg.h"
#include "opt.h"

#pragma mark start

void
defframe(Prog *ptxt)
{
	// fill in argument size
	ptxt->to.type = D_CONST2;
	ptxt->reg = 0; // flags
	ptxt->to.offset2 = rnd(curfn->type->argwid, widthptr);
	
}

struct mystruct {
     int fred[5];
     char wilma, betty;
     float barny=1;
};

void ginscall(Node *f, int proc) {
	Prog *p;
	Node n1, r, con;

	switch(proc) {
	default:
		fatal("ginscall: bad proc %d", proc);
		break;

	case 0:	// normal call
		p = gins(ABL, N, f);
		afunclit(&p->to);
		break;

	case 1:	// call in new proc (go)
	case 2:	// deferred call (defer)
		regalloc(&r, types[tptr], N);
		p = gins(AMOVW, N, &r);
		p->from.type = D_OREG;
		p->from.reg = REGSP;

		if(proc == 2) {
			nodconst(&con, types[TINT32], 0);
			p = gins(ACMP, &con, N);
			p->reg = 0;
			patch(gbranch(ABNE, T), retpc);
		}
		break;
	}
}

// Example of a section break
#pragma mark -

/*
 * n is call to interface method.
 * generate res = n.
 */
void
cgen_callinter(Node *n, Node *res, int proc)
{
	int r;
	Node *i, *f;
	Node tmpi, nodo, nodr, nodsp;

	i = n->left;
	if(i->op != ODOTINTER)
		fatal("cgen_callinter: not ODOTINTER %O", i->op);

	f = i->right;		// field
	if(f->op != ONAME)
		fatal("cgen_callinter: not ONAME %O", f->op);

	i = i->left;		// interface

	// Release res register during genlist and cgen,
	// which might have their own function calls.
	r = -1;
	if(res != N && (res->op == OREGISTER || res->op == OINDREG)) {
		r = res->val.u.reg;
		reg[r]--;
	}

	if(!i->addable) {
		tempname(&tmpi, i->type);
		cgen(i, &tmpi);
		i = &tmpi;
	}

	genlist(n->list);			// args
	if(r >= 0)
		reg[r]++;

	regalloc(&nodr, types[tptr], res);
	regalloc(&nodo, types[tptr], &nodr);
	nodo.op = OINDREG;

	agen(i, &nodr);		// REG = &inter

	nodindreg(&nodsp, types[tptr], REGSP);
	nodsp.xoffset = 4;
	nodo.xoffset += widthptr;
	cgen(&nodo, &nodsp);	// 4(SP) = 8(REG) -- i.s

	nodo.xoffset -= widthptr;
	cgen(&nodo, &nodr);	// REG = 0(REG) -- i.m

	nodo.xoffset = n->left->xoffset + 3*widthptr + 8;
	cgen(&nodo, &nodr);	// REG = 32+offset(REG) -- i.m->fun[f]

	// BOTCH nodr.type = fntype;
	nodr.type = n->left->type;
	ginscall(&nodr, proc);

	regfree(&nodr);
	regfree(&nodo);

	setmaxarg(n->left->type);
}

/*
 * generate function call;
 *	proc=0	normal call
 *	proc=1	goroutine run in new proc
 *	proc=2	defer call save away stack
 */
void
cgen_call(Node *n, int proc)
{
	Type *t;
	Node nod, afun;

	if(n == N)
		return;

	if(n->left->ullman >= UINF) {
		// if name involves a fn call
		// precompute the address of the fn
		tempname(&afun, types[tptr]);
		cgen(n->left, &afun);
	}

	genlist(n->list);		// assign the args
	t = n->left->type;

	setmaxarg(t);

	// call tempname pointer
	if(n->left->ullman >= UINF) {
		regalloc(&nod, types[tptr], N);
		cgen_as(&nod, &afun);
		nod.type = t;
		ginscall(&nod, proc);
		regfree(&nod);
		goto ret;
	}

	// call pointer
	if(n->left->op != ONAME || n->left->class != PFUNC) {
		regalloc(&nod, types[tptr], N);
		cgen_as(&nod, n->left);
		nod.type = t;
		ginscall(&nod, proc);
		regfree(&nod);
		goto ret;
	}

	// call direct
	n->left->method = 1;
	ginscall(n->left, proc);


ret:
	;
}

/*
 * call to n has already been generated.
 * generate:
 *	res = return value from call.
 */
void
cgen_callret(Node *n, Node *res)
{
	Node nod;
	Type *fp, *t;
	Iter flist;

	t = n->left->type;
	if(t->etype == TPTR32 || t->etype == TPTR64)
		t = t->type;

	fp = structfirst(&flist, getoutarg(t));
	if(fp == T)
		fatal("cgen_callret: nil");

	memset(&nod, 0, sizeof(nod));
	nod.op = OINDREG;
	nod.val.u.reg = REGSP;
	nod.addable = 1;

	nod.xoffset = fp->width + 4; // +4: saved lr at 0(SP)
	nod.type = fp->type;
	cgen_as(res, &nod);
}


#pragma mark Keywords

auto
double
int
long
break
else
long
switch
case
enum
register
typedef
char
extern
return
union
const
float
short
unsigned
continue
for
signed
void
default
goto
sizeof
volatile
do
if
static
while

