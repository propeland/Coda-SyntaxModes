/*
  Don't try to make sense of (or compile!) this code, it doesn't
  make sense. It's a variety of pasted C code that I've found to
  push the boundaries of the syntax coloring
*/

#undef	  EXTERN
#define	  EXTERN

#include "gg.h"
#include "opt.h"


#pragma mark hello

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

/*
 * call to n has already been generated.
 * generate:
 *	res = &return value from call.
 */
void
cgen_aret(Node *n, Node *res)
{
	Node nod1, nod2;
	Type *fp, *t;
	Iter flist;

	t = n->left->type;
	if(isptr[t->etype])
		t = t->type;

	fp = structfirst(&flist, getoutarg(t));
	if(fp == T)
		fatal("cgen_aret: nil");

	memset(&nod1, 0, sizeof(nod1));
	nod1.op = OINDREG;
	nod1.val.u.reg = REGSP;
	nod1.addable = 1;

	nod1.xoffset = fp->width + 4; // +4: saved lr at 0(SP)
	nod1.type = fp->type;

	if(res->op != OREGISTER) {
		regalloc(&nod2, types[tptr], res);
		agen(&nod1, &nod2);
		gins(AMOVW, &nod2, res);
		regfree(&nod2);
	} else
		agen(&nod1, res);
}

/*
 * generate return.
 * n->left is assignments to return values.
 */
void
cgen_ret(Node *n)
{
	genlist(n->list);		// copy out args
	if(hasdefer || curfn->exit)
		gjmp(retpc);
	else
		gins(ARET, N, N);
}

/*
 * generate += *= etc.
 */
void
cgen_asop(Node *n)
{
	Node n1, n2, n3, n4;
	Node *nl, *nr;
	Prog *p1;
	Addr addr;
	int a, w;

	nl = n->left;
	nr = n->right;

	if(nr->ullman >= UINF && nl->ullman >= UINF) {
		tempname(&n1, nr->type);
		cgen(nr, &n1);
		n2 = *n;
		n2.right = &n1;
		cgen_asop(&n2);
		goto ret;
	}

	if(!isint[nl->type->etype])
		goto hard;
	if(!isint[nr->type->etype])
		goto hard;
	if(is64(nl->type) || is64(nr->type))
		goto hard64;

	switch(n->etype) {
	case OADD:
	case OSUB:
	case OXOR:
	case OAND:
	case OOR:
		a = optoas(n->etype, nl->type);
		if(nl->addable) {
			regalloc(&n3, nr->type, N);
			cgen(nr, &n3);
			regalloc(&n2, nl->type, N);
			cgen(nl, &n2);
			gins(a, &n3, &n2);
			cgen(&n2, nl);
			regfree(&n2);
			regfree(&n3);
			goto ret;
		}
		if(nr->ullman < UINF)
		if(sudoaddable(a, nl, &addr, &w)) {
			w = optoas(OAS, nl->type);
			regalloc(&n2, nl->type, N);
			p1 = gins(w, N, &n2);
			p1->from = addr;
			regalloc(&n3, nr->type, N);
			cgen(nr, &n3);
			gins(a, &n3, &n2);
			p1 = gins(w, &n2, N);
			p1->to = addr;
			regfree(&n2);
			regfree(&n3);
			sudoclean();
			goto ret;
		}
	}

hard:
	n2.op = 0;
	n1.op = 0;
	if(nr->ullman >= nl->ullman || nl->addable) {
		regalloc(&n2, nr->type, N);
		cgen(nr, &n2);
		nr = &n2;
	} else {
		tempname(&n2, nr->type);
		cgen(nr, &n2);
		nr = &n2;
	}
	if(!nl->addable) {
		igen(nl, &n1, N);
		nl = &n1;
	}

	n3 = *n;
	n3.left = nl;
	n3.right = nr;
	n3.op = n->etype;

	regalloc(&n4, nl->type, N);
	cgen(&n3, &n4);
	gmove(&n4, nl);

	if(n1.op)
		regfree(&n1);
	if(n2.op == OREGISTER)
		regfree(&n2);
	regfree(&n4);
	goto ret;

hard64:
	if(nr->ullman > nl->ullman) {
		tempname(&n2, nr->type);
		cgen(nr, &n2);
		igen(nl, &n1, N);
	} else {
		igen(nl, &n1, N);
		tempname(&n2, nr->type);
		cgen(nr, &n2);
	}

	n3 = *n;
	n3.left = &n1;
	n3.right = &n2;
	n3.op = n->etype;

	cgen(&n3, &n1);

ret:
	;
}

int
samereg(Node *a, Node *b)
{
	if(a->op != OREGISTER)
		return 0;
	if(b->op != OREGISTER)
		return 0;
	if(a->val.u.reg != b->val.u.reg)
		return 0;
	return 1;
}

/*
 * generate shift according to op, one of:
 *	res = nl << nr
 *	res = nl >> nr
 */
void
cgen_shift(int op, Node *nl, Node *nr, Node *res)
{
	Node n1, n2, n3, t;
	int w;
	Prog *p1, *p2, *p3;
	uvlong sc;

	if(nl->type->width > 4)
		fatal("cgen_shift %T", nl->type);

	w = nl->type->width * 8;

	if(nr->op == OLITERAL) {
		regalloc(&n1, nl->type, res);
		cgen(nl, &n1);
		sc = mpgetfix(nr->val.u.xval);
		if(sc == 0) {
			// nothing to do
		} else if(sc >= nl->type->width*8) {
			if(op == ORSH && issigned[nl->type->etype])
				gshift(AMOVW, &n1, SHIFT_AR, w, &n1);
			else
				gins(AEOR, &n1, &n1);
		} else {
			if(op == ORSH && issigned[nl->type->etype])
				gshift(AMOVW, &n1, SHIFT_AR, sc, &n1);
			else if(op == ORSH)
				gshift(AMOVW, &n1, SHIFT_LR, sc, &n1);
			else // OLSH
				gshift(AMOVW, &n1, SHIFT_LL, sc, &n1);
		}
		gmove(&n1, res);
		regfree(&n1);
		return;
	}

	if(nl->ullman >= nr->ullman) {
		regalloc(&n2, nl->type, res);
		cgen(nl, &n2);
		regalloc(&n1, nr->type, N);
		cgen(nr, &n1);
	} else {
		regalloc(&n1, nr->type, N);
		cgen(nr, &n1);
		regalloc(&n2, nl->type, res);
		cgen(nl, &n2);
	}

	// test for shift being 0
	p1 = gins(ATST, &n1, N);
	p3 = gbranch(ABEQ, T);

	// test and fix up large shifts
	regalloc(&n3, nr->type, N);
	nodconst(&t, types[TUINT32], w);
	gmove(&t, &n3);
	gcmp(ACMP, &n1, &n3);
	if(op == ORSH) {
		if(issigned[nl->type->etype]) {
			p1 = gshift(AMOVW, &n2, SHIFT_AR, w-1, &n2);
			p2 = gregshift(AMOVW, &n2, SHIFT_AR, &n1, &n2);
		} else {
			p1 = gins(AEOR, &n2, &n2);
			p2 = gregshift(AMOVW, &n2, SHIFT_LR, &n1, &n2);
		}
		p1->scond = C_SCOND_HS;
		p2->scond = C_SCOND_LO;
	} else {
		p1 = gins(AEOR, &n2, &n2);
		p2 = gregshift(AMOVW, &n2, SHIFT_LL, &n1, &n2);
		p1->scond = C_SCOND_HS;
		p2->scond = C_SCOND_LO;
	}
	regfree(&n3);

	patch(p3, pc);
	gmove(&n2, res);

	regfree(&n1);
	regfree(&n2);
}