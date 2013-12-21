/* sim-safe.c - sample functional simulator implementation */

/* SimpleScalar(TM) Tool Suite
 * Copyright (C) 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 * All Rights Reserved. 
 * 
 * THIS IS A LEGAL DOCUMENT, BY USING SIMPLESCALAR,
 * YOU ARE AGREEING TO THESE TERMS AND CONDITIONS.
 * 
 * No portion of this work may be used by any commercial entity, or for any
 * commercial purpose, without the prior, written permission of SimpleScalar,
 * LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted
 * as described below.
 * 
 * 1. SimpleScalar is provided AS IS, with no warranty of any kind, express
 * or implied. The user of the program accepts full responsibility for the
 * application of the program and the use of any results.
 * 
 * 2. Nonprofit and noncommercial use is encouraged. SimpleScalar may be
 * downloaded, compiled, executed, copied, and modified solely for nonprofit,
 * educational, noncommercial research, and noncommercial scholarship
 * purposes provided that this notice in its entirety accompanies all copies.
 * Copies of the modified software can be delivered to persons who use it
 * solely for nonprofit, educational, noncommercial research, and
 * noncommercial scholarship purposes provided that this notice in its
 * entirety accompanies all copies.
 * 
 * 3. ALL COMMERCIAL USE, AND ALL USE BY FOR PROFIT ENTITIES, IS EXPRESSLY
 * PROHIBITED WITHOUT A LICENSE FROM SIMPLESCALAR, LLC (info@simplescalar.com).
 * 
 * 4. No nonprofit user may place any restrictions on the use of this software,
 * including as modified by the user, by any other authorized user.
 * 
 * 5. Noncommercial and nonprofit users may distribute copies of SimpleScalar
 * in compiled or executable form as set forth in Section 2, provided that
 * either: (A) it is accompanied by the corresponding machine-readable source
 * code, or (B) it is accompanied by a written offer, with no time limit, to
 * give anyone a machine-readable copy of the corresponding source code in
 * return for reimbursement of the cost of distribution. This written offer
 * must permit verbatim duplication by anyone, or (C) it is distributed by
 * someone who received only the executable form, and is accompanied by a
 * copy of the written offer of source code.
 * 
 * 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
 * currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
 * 2395 Timbercrest Court, Ann Arbor, MI 48105.
 * 
 * Copyright (C) 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "options.h"
#include "stats.h"
#include "sim.h"


/* ECE552 Assignment 1 - STATS COUNTERS - BEGIN */
static counter_t sim_num_RAW_hazard_q1;
static counter_t sim_num_RAW_hazard_q2;
static counter_t sim_num_WAW_hazard_q3;
static counter_t sim_num_structural_hazard_q3;
static counter_t sim_num_one_cycle_hazard_q1;
static counter_t sim_num_two_cycle_hazard_q1;
static counter_t sim_num_one_cycle_hazard_q2;
static counter_t sim_num_two_cycle_hazard_q2;
static counter_t sim_num_one_cycle_hazard_q3;
static counter_t sim_num_two_cycle_hazard_q3;
/* ECE552 Assignment 1 - STATS COUNTERS - END */

/*
 * This file implements a functional simulator.  This functional simulator is
 * the simplest, most user-friendly simulator in the simplescalar tool set.
 * Unlike sim-fast, this functional simulator checks for all instruction
 * errors, and the implementation is crafted for clarity rather than speed.
 */

/* simulated registers */
static struct regs_t regs;

/* simulated memory */
static struct mem_t *mem = NULL;

/* track number of refs */
static counter_t sim_num_refs = 0;

/* ECE552 Assignment 1 - BEGIN CODE */

/* reg_counters and bool type declared*/

typedef enum 
{
 false, true 
} bool;

static counter_t reg_ready_q1[MD_TOTAL_REGS];
static counter_t reg_ready_q2[MD_TOTAL_REGS];
static counter_t reg_ready_q3[MD_TOTAL_REGS];
static counter_t write_back_ready = 0;
bool is_previous_load = false;
bool is_current_load = false;

/* ECE552 Assignment 1 - END CODE */

/* maximum number of inst's to execute */
static unsigned int max_insts;

/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)
{
  opt_reg_header(odb, 
"sim-safe: This simulator implements a functional simulator.  This\n"
"functional simulator is the simplest, most user-friendly simulator in the\n"
"simplescalar tool set.  Unlike sim-fast, this functional simulator checks\n"
"for all instruction errors, and the implementation is crafted for clarity\n"
"rather than speed.\n"
		 );

  /* instruction limit */
  opt_reg_uint(odb, "-max:inst", "maximum number of inst's to execute",
	       &max_insts, /* default */0,
	       /* print */TRUE, /* format */NULL);

}

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb, int argc, char **argv)
{
  /* nada */
}

/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb)
{
  stat_reg_counter(sdb, "sim_num_insn",
		   "total number of instructions executed",
		   &sim_num_insn, sim_num_insn, NULL);
  stat_reg_counter(sdb, "sim_num_refs",
		   "total number of loads and stores executed",
		   &sim_num_refs, 0, NULL);
  stat_reg_int(sdb, "sim_elapsed_time",
	       "total simulation time in seconds",
	       &sim_elapsed_time, 0, NULL);
  stat_reg_formula(sdb, "sim_inst_rate",
		   "simulation speed (in insts/sec)",
		   "sim_num_insn / sim_elapsed_time", NULL);

  /* ECE552 Assignment 1 - BEGIN CODE */

  stat_reg_counter(sdb, "sim_num_RAW_hazard_q1",
		   "total number of RAW hazards (q1)",
		   &sim_num_RAW_hazard_q1, sim_num_RAW_hazard_q1, NULL);

  stat_reg_counter(sdb, "sim_num_one_cycle_hazard_q1",
		   "total one cycle hazard q1",
		   &sim_num_one_cycle_hazard_q1, sim_num_one_cycle_hazard_q1, NULL);

  stat_reg_counter(sdb, "sim_num_two_cycle_hazard_q1",
		   "total two cycle hazard q1",
		   &sim_num_two_cycle_hazard_q1, sim_num_two_cycle_hazard_q1, NULL);

  stat_reg_counter(sdb, "sim_num_one_cycle_hazard_q2",
		   "total one cycle hazard q2",
		   &sim_num_one_cycle_hazard_q2, sim_num_one_cycle_hazard_q2, NULL);

  stat_reg_counter(sdb, "sim_num_two_cycle_hazard_q2",
		   "total two cycle hazard q2",
		   &sim_num_two_cycle_hazard_q2, sim_num_two_cycle_hazard_q2, NULL); 

  stat_reg_counter(sdb, "sim_num_one_cycle_hazard_q3",
		   "total one cycle hazard q3",
		   &sim_num_one_cycle_hazard_q3, sim_num_one_cycle_hazard_q3, NULL);
  
  stat_reg_counter(sdb, "sim_num_two_cycle_hazard_q3",
		   "total two cycle hazard q3",
		   &sim_num_two_cycle_hazard_q3, sim_num_two_cycle_hazard_q3, NULL);

  stat_reg_counter(sdb, "sim_num_RAW_hazard_q2",
		   "total number of RAW hazards (q2)",
		   &sim_num_RAW_hazard_q2, sim_num_RAW_hazard_q2, NULL);

  stat_reg_counter(sdb, "sim_num_WAW_hazard_q3",
		   "total number of WAW hazards (q3)",
		   &sim_num_WAW_hazard_q3, sim_num_WAW_hazard_q3, NULL);

  /* These are structural hazards occuring when two instructions **without** a WAW dependence
     try to do the WriteBack in the same cycle.  The latest instruction should stall.
   */
  stat_reg_counter(sdb, "sim_num_structural_hazard_q3",
		   "total number of structural hazards (q3)",
		   &sim_num_structural_hazard_q3, sim_num_structural_hazard_q3, NULL);

  stat_reg_formula(sdb, "CPI_from_RAW_hazard_q1",
		   "CPI from RAW hazard (q1)",
		   "1 + (sim_num_one_cycle_hazard_q1/sim_num_insn)+ ((sim_num_two_cycle_hazard_q1/sim_num_insn)*2)", NULL);

  stat_reg_formula(sdb, "CPI_from_RAW_hazard_q2",
		   "CPI from RAW hazard (q2)",
		   "1 +  (sim_num_one_cycle_hazard_q2/sim_num_insn) + ((sim_num_two_cycle_hazard_q2/sim_num_insn)*2)", NULL);

  /* Include both WAW and structural hazards in your CPI computation */
  stat_reg_formula(sdb, "CPI_from_WAW_and_Structural_hazard_q3",
		   "CPI from WAW and structural hazards (q3)",
		   "1 + ((sim_num_one_cycle_hazard_q3 + sim_num_structural_hazard_q3)/sim_num_insn) + ((sim_num_two_cycle_hazard_q3/sim_num_insn)*2)",  NULL);

  /* ECE552 Assignment 1 - END CODE */

  ld_reg_stats(sdb);
  mem_reg_stats(mem, sdb);
}

/* initialize the simulator */
void
sim_init(void)
{
  sim_num_refs = 0;

  /* allocate and initialize register file */
  regs_init(&regs);

  /* allocate and initialize memory space */
  mem = mem_create("mem");
  mem_init(mem);
}

/* load program into simulated state */
void
sim_load_prog(char *fname,		/* program to load */
	      int argc, char **argv,	/* program arguments */
	      char **envp)		/* program environment */
{
  /* load program text and data, set up environment, memory, and regs */
  ld_load_prog(fname, argc, argv, envp, &regs, mem, TRUE);

  /* initialize the DLite debugger */
  dlite_init(md_reg_obj, dlite_mem_obj, dlite_mstate_obj);
}

/* print simulator-specific configuration information */
void
sim_aux_config(FILE *stream)		/* output stream */
{
  /* nothing currently */
}

/* dump simulator-specific auxiliary simulator statistics */
void
sim_aux_stats(FILE *stream)		/* output stream */
{
  /* nada */
}

/* un-initialize simulator-specific state */
void
sim_uninit(void)
{
  /* nada */
}


/*
 * configure the execution engine
 */

/*
 * precise architected register accessors
 */

/* next program counter */
#define SET_NPC(EXPR)		(regs.regs_NPC = (EXPR))

/* current program counter */
#define CPC			(regs.regs_PC)

/* general purpose registers */
#define GPR(N)			(regs.regs_R[N])
#define SET_GPR(N,EXPR)		(regs.regs_R[N] = (EXPR))

#define DNA (0)

#if defined(TARGET_PISA)

/* general register dependence decoders */
#define DGPR(N)			(N)
#define DGPR_D(N)		((N) &~1)

/* floating point register dependence decoders */
#define DFPR_L(N)		(((N)+32)&~1)
#define DFPR_F(N)		(((N)+32)&~1)
#define DFPR_D(N)		(((N)+32)&~1)

/* miscellaneous register dependence decoders */
#define DHI			(0+32+32)
#define DLO			(1+32+32)
#define DFCC			(2+32+32)
#define DTMP			(3+32+32)

/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_L(N)		(regs.regs_F.l[(N)])
#define SET_FPR_L(N,EXPR)	(regs.regs_F.l[(N)] = (EXPR))
#define FPR_F(N)		(regs.regs_F.f[(N)])
#define SET_FPR_F(N,EXPR)	(regs.regs_F.f[(N)] = (EXPR))
#define FPR_D(N)		(regs.regs_F.d[(N) >> 1])
#define SET_FPR_D(N,EXPR)	(regs.regs_F.d[(N) >> 1] = (EXPR))

/* miscellaneous register accessors */
#define SET_HI(EXPR)		(regs.regs_C.hi = (EXPR))
#define HI			(regs.regs_C.hi)
#define SET_LO(EXPR)		(regs.regs_C.lo = (EXPR))
#define LO			(regs.regs_C.lo)
#define FCC			(regs.regs_C.fcc)
#define SET_FCC(EXPR)		(regs.regs_C.fcc = (EXPR))

#elif defined(TARGET_ALPHA)

/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_Q(N)		(regs.regs_F.q[N])
#define SET_FPR_Q(N,EXPR)	(regs.regs_F.q[N] = (EXPR))
#define FPR(N)			(regs.regs_F.d[(N)])
#define SET_FPR(N,EXPR)		(regs.regs_F.d[(N)] = (EXPR))

/* miscellaneous register accessors */
#define FPCR			(regs.regs_C.fpcr)
#define SET_FPCR(EXPR)		(regs.regs_C.fpcr = (EXPR))
#define UNIQ			(regs.regs_C.uniq)
#define SET_UNIQ(EXPR)		(regs.regs_C.uniq = (EXPR))

#else
#error No ISA target defined...
#endif

/* precise architected memory state accessor macros */
#define READ_BYTE(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_BYTE(mem, addr))
#define READ_HALF(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_HALF(mem, addr))
#define READ_WORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_WORD(mem, addr))
#ifdef HOST_HAS_QWORD
#define READ_QWORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_QWORD(mem, addr))
#endif /* HOST_HAS_QWORD */

#define WRITE_BYTE(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_BYTE(mem, addr, (SRC)))
#define WRITE_HALF(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_HALF(mem, addr, (SRC)))
#define WRITE_WORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_WORD(mem, addr, (SRC)))
#ifdef HOST_HAS_QWORD
#define WRITE_QWORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_QWORD(mem, addr, (SRC)))
#endif /* HOST_HAS_QWORD */

/* system call handler macro */
#define SYSCALL(INST)	sys_syscall(&regs, mem_access, mem, INST, TRUE)

/* start simulation, program loaded, processor precise state initialized */
void
sim_main(void)
{
  md_inst_t inst;
  register md_addr_t addr;
  enum md_opcode op;
  register int is_write;
  enum md_fault_type fault;

  /* ECE552 Assignment 1 - BEGIN CODE */

  int r_in[3], r_out[2];

  /* ECE552 Assignment 1 - END CODE */

  fprintf(stderr, "sim: ** starting functional simulation **\n");

  /* set up initial default next PC */
  regs.regs_NPC = regs.regs_PC + sizeof(md_inst_t);

  /* check for DLite debugger entry condition */
  if (dlite_check_break(regs.regs_PC, /* !access */0, /* addr */0, 0, 0))
    dlite_main(regs.regs_PC - sizeof(md_inst_t),
	       regs.regs_PC, sim_num_insn, &regs, mem);

  while (TRUE)
    {

      /* maintain $r0 semantics */
      regs.regs_R[MD_REG_ZERO] = 0;
#ifdef TARGET_ALPHA
      regs.regs_F.d[MD_REG_ZERO] = 0.0;
#endif /* TARGET_ALPHA */

      /* get the next instruction to execute */
      MD_FETCH_INST(inst, mem, regs.regs_PC);

      /* keep an instruction count */
      sim_num_insn++;

      /* set default reference address and access mode */
      addr = 0; is_write = FALSE;

      /* set default fault - none */
      fault = md_fault_none;

      /* decode the instruction */
      MD_SET_OPCODE(op, inst);

      /* execute the instruction */

      switch (op)
	{
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3)		\
	case OP:							\
	  r_out[0] = (O1); r_out[1] = (O2);				\
	  r_in[0] = (I1); r_in[1] = (I2); r_in[2] = (I3);		\
          SYMCAT(OP,_IMPL);						\
          break;
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
        case OP:							\
          panic("attempted to execute a linking opcode");
#define CONNECT(OP)
#define DECLARE_FAULT(FAULT)						\
	  { fault = (FAULT); break; }
#include "machine.def"
	default:
	  panic("attempted to execute a bogus opcode");
      }
      
      /* ECE552 Assignment 1 - BEGIN CODE */

      /* ECE552 Part 1 */

      int i, counter_q1 = 0, counter_q2 = 0, counter_q3 = 0;

      for (i = 0;  i < 3; i++)
      {
	if (r_in[i] != DNA && reg_ready_q1[r_in[i]] > sim_num_insn)
	{
	  int cycle = reg_ready_q1[r_in[i]] - sim_num_insn;
	  
	  if (cycle ==  1)
	  {
  	    counter_q1++;
	  }
	  else if (cycle == 2)
	  {
	    counter_q1 =  counter_q1 + 2;
	  }

	  //reset the register
	  reg_ready_q1[r_in[i]] = DNA;
	}
      }

      if (counter_q1 == 1)
      {
        sim_num_one_cycle_hazard_q1++;
      }
      else if (counter_q1 >= 2)
      {
	 sim_num_two_cycle_hazard_q1++;
      }	

      if (r_out[0] != DNA)
      {
	reg_ready_q1[r_out[0]] = sim_num_insn + 3;
      }

      if (r_out[1] != DNA)
      {
	reg_ready_q1[r_out[1]] = sim_num_insn + 3;
      }
    
      sim_num_RAW_hazard_q1 = sim_num_one_cycle_hazard_q1 + sim_num_two_cycle_hazard_q1; 

      /* ECE552 Part 2*/

      for (i = 0;  i < 3; i++)
      {
	if (r_in[i] != DNA && reg_ready_q2[r_in[i]] > sim_num_insn)
	{
	  if ((i == 0) && (MD_OP_FLAGS(op) & F_MEM) &&
	      (MD_OP_FLAGS(op) & F_STORE))
	  {
	      continue;
	  }
          
	  int cycle = reg_ready_q2[r_in[i]] - sim_num_insn;

	  if (cycle == 1)
	  {
	    counter_q2++;
	  }
	  else if (cycle == 2)
	  {
		counter_q2 = counter_q2 + 2;
	  }

          //reset the register
	  reg_ready_q2[r_in[i]] = DNA;
	}
      }
      
      if (counter_q2 == 1)
      {
	sim_num_one_cycle_hazard_q2++;
      }
      else if (counter_q2 >= 2)
      {
	sim_num_two_cycle_hazard_q2++;
      }

      if ((MD_OP_FLAGS(op) & F_MEM) && (MD_OP_FLAGS(op) & F_LOAD))
      {
        if (r_out[0] != DNA)
        {
	  reg_ready_q2[r_out[0]] = sim_num_insn + 3;
        }

        if (r_out[1] != DNA)
        {
	  reg_ready_q2[r_out[1]] = sim_num_insn + 3;
        }
      }
      else if (!(MD_OP_FLAGS(op) & F_MEM))
      {
        if (r_out[0] != DNA)
        {
	  reg_ready_q2[r_out[0]] = sim_num_insn + 2;
        }

        if (r_out[1] != DNA)
        {
	  reg_ready_q2[r_out[1]] = sim_num_insn + 2;
        }
      }

      sim_num_RAW_hazard_q2 = sim_num_one_cycle_hazard_q2 + sim_num_two_cycle_hazard_q2;

      /* ECE552 Part 3 */


      if ((MD_OP_FLAGS(op) & F_MEM) && (MD_OP_FLAGS(op) & F_LOAD))
      {
	is_current_load = true;
      }
      else if (!(MD_OP_FLAGS(op) & F_MEM) && (MD_OP_FLAGS(op) & F_LOAD))
      {
	is_previous_load = false;
      }
 
      if (!(MD_OP_FLAGS(op) & F_LOAD))
      {
		
        for (i = 0;  i < 2; i++)
        {
	  if (r_out[i] != DNA && reg_ready_q3[r_out[i]] > sim_num_insn)
	  {
   	    int cycle = reg_ready_q3[r_out[i]] - sim_num_insn;
	  
	    if (cycle == 1)
	    {
	      //two cycle stall for non mem instruction
	      if (is_current_load == true && is_current_load == true)
	      {
		 sim_num_two_cycle_hazard_q3++;
	      }
	      else
	      {
		sim_num_one_cycle_hazard_q3++;	
	      }
	  }
   	  else if (cycle == 2)
	  {
	    sim_num_two_cycle_hazard_q3++;
	  }

 	  reg_ready_q3[r_out[i]] = DNA;
          counter_q3 = 1;
        }
      } 

	if(counter_q3 == 0 && (r_out[0] != DNA || r_out[1] != DNA))
	{
	  if(is_current_load && is_previous_load && write_back_ready == sim_num_insn + 1)
          {
		sim_num_structural_hazard_q3 = sim_num_structural_hazard_q3 + 2;
          }
	  else if(write_back_ready == sim_num_insn)
	  {
	    sim_num_structural_hazard_q3++;
	  }
	}
    }
      
     if ((MD_OP_FLAGS(op) & F_MEM) && (MD_OP_FLAGS(op) & F_LOAD))
     {
       if (r_out[0] != DNA)
       {
	 reg_ready_q3[r_out[0]] = sim_num_insn + 3;
       }

       if (r_out[1] != DNA)
       {
	 reg_ready_q3[r_out[1]] = sim_num_insn + 3;
       }

       write_back_ready = sim_num_insn + 2;
       is_previous_load = true;
     }
     else if (!(MD_OP_FLAGS(op) & F_MEM) && (MD_OP_FLAGS(op) & F_LOAD))
     {
	is_previous_load = false;
     }

     sim_num_WAW_hazard_q3  = sim_num_one_cycle_hazard_q3 + sim_num_two_cycle_hazard_q3;

     /* ECE552 Assignment 1 - END CODE */

      if (fault != md_fault_none)
	fatal("fault (%d) detected @ 0x%08p", fault, regs.regs_PC);

      if (verbose)
	{
	  myfprintf(stderr, "%10n [xor: 0x%08x] @ 0x%08p: ",
		    sim_num_insn, md_xor_regs(&regs), regs.regs_PC);
	  md_print_insn(inst, regs.regs_PC, stderr);
	  if (MD_OP_FLAGS(op) & F_MEM)
	    myfprintf(stderr, "  mem: 0x%08p", addr);
	  fprintf(stderr, "\n");
	  /* fflush(stderr); */
	}

      if (MD_OP_FLAGS(op) & F_MEM)
	{
	  sim_num_refs++;
	  if (MD_OP_FLAGS(op) & F_STORE)
	    is_write = TRUE;
	}

      /* check for DLite debugger entry condition */
      if (dlite_check_break(regs.regs_NPC,
			    is_write ? ACCESS_WRITE : ACCESS_READ,
			    addr, sim_num_insn, sim_num_insn))
	dlite_main(regs.regs_PC, regs.regs_NPC, sim_num_insn, &regs, mem);

      /* go to the next instruction */
      regs.regs_PC = regs.regs_NPC;
      regs.regs_NPC += sizeof(md_inst_t);

      /* finish early? */
      if (max_insts && sim_num_insn >= max_insts)
	return;
    }
}