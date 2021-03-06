/*
 * SchemeSmob.c
 *
 * Scheme small objects (SMOBS) for opencog -- core functions.
 *
 * Copyright (c) 2008, 2013, 2014 Linas Vepstas <linas@linas.org>
 */

#ifdef HAVE_GUILE

#include <libguile.h>

#include <opencog/atomspace/AtomSpace.h>
#include "SchemePrimitive.h"
#include "SchemeSmob.h"

using namespace opencog;

/**
 * Just one scheme smob type is used to implement the interface.
 *
 * The cog_misc_tag is used to store all structures, such as atoms
 * and truth values. It is assumed that these structures are all
 * ephemeral (garbage-collected), including the Handles.  Note that
 * atoms in the atomspace have a concrete existence outside of the
 * scheme shell. By contrast, truth values created by the scheme
 * shell are garbage collected by the shell.
 *
 * The type of the "misc" structure is stored in the flag bits;
 * thus, handling is dispatched based on these flags.
 *
 * XXX TODO:
 * The cog_misc_tag should be replaced by a tag-per-class (i.e. we
 * should have a separate tag for handles, tv's, etc.) This would
 * simplify that code, and probably improve performance just a bit.
 */

scm_t_bits SchemeSmob::cog_misc_tag;
bool SchemeSmob::is_inited = false;
SCM SchemeSmob::_radix_ten;

void SchemeSmob::init()
{
	// XXX It would be ever so slightly more correct to use
	// pthread_once() here, but that currently seems like overkill.
	if (!is_inited)
	{
		is_inited = true;
		init_smob_type();
		register_procs();

		atomspace_fluid = scm_make_fluid();
		atomspace_fluid = scm_permanent_object(atomspace_fluid);
		_radix_ten = scm_from_int8(10);
	}
}

SchemeSmob::SchemeSmob()
{
	init();
}

/* ============================================================== */

void SchemeSmob::init_smob_type(void)
{
	// A SMOB type for everything, incuding atoms.
	cog_misc_tag = scm_make_smob_type ("opencog-misc", sizeof (scm_t_bits));
	scm_set_smob_print (cog_misc_tag, print_misc);
	scm_set_smob_equalp (cog_misc_tag, equalp_misc);
	// scm_set_smob_mark (cog_misc_tag, mark_misc);
	scm_set_smob_free (cog_misc_tag, free_misc);
}

/* ============================================================== */

SCM SchemeSmob::equalp_misc(SCM a, SCM b)
{
	// If they're not something we know about, let scheme sort it out.
	// (Actualy, this should never happen ...)
	if (not SCM_SMOB_PREDICATE(SchemeSmob::cog_misc_tag, a))
		return scm_equal_p(a, b);

	// If the types don't match, they can't be equal.
	scm_t_bits ta = SCM_SMOB_FLAGS(a);
	scm_t_bits tb = SCM_SMOB_FLAGS(b);
	if (ta != tb)
		return SCM_BOOL_F;

	switch (ta)
	{
		default: // Should never happen.
		case 0:  // Should never happen.
			return SCM_BOOL_F;
		case COG_AS:
		{
			AtomSpace* as = (AtomSpace *) SCM_SMOB_DATA(a);
			AtomSpace* bs = (AtomSpace *) SCM_SMOB_DATA(b);
			scm_remember_upto_here_1(a);
			scm_remember_upto_here_1(b);
			/* Just a simple pointer comparison */
			if (as == bs) return SCM_BOOL_T;
			return SCM_BOOL_F;
		}
		case COG_AV:
		{
			AttentionValue* av = (AttentionValue *) SCM_SMOB_DATA(a);
			AttentionValue* bv = (AttentionValue *) SCM_SMOB_DATA(b);
			scm_remember_upto_here_1(a);
			scm_remember_upto_here_1(b);
			if (av == bv) return SCM_BOOL_T;
			if (*av == *bv) return SCM_BOOL_T;
			return SCM_BOOL_F;
		}
		case COG_EXTEND:
		{
			// We compare pointers here, only.
			PrimitiveEnviron* av = (PrimitiveEnviron *) SCM_SMOB_DATA(a);
			PrimitiveEnviron* bv = (PrimitiveEnviron *) SCM_SMOB_DATA(b);
			scm_remember_upto_here_1(a);
			scm_remember_upto_here_1(b);
			if (av == bv) return SCM_BOOL_T;
			return SCM_BOOL_F;
		}
		case COG_HANDLE:
		{
			Handle ha(scm_to_handle(a));
			Handle hb(scm_to_handle(b));
			if (ha == hb) return SCM_BOOL_T;
			return SCM_BOOL_F;
		}
		case COG_TV:
		{
			TruthValue* av = (TruthValue *) SCM_SMOB_DATA(a);
			TruthValue* bv = (TruthValue *) SCM_SMOB_DATA(b);
			scm_remember_upto_here_1(a);
			scm_remember_upto_here_1(b);
			if (av == bv) return SCM_BOOL_T;
			if (*av == *bv) return SCM_BOOL_T;
			return SCM_BOOL_F;
		}
	}
}

/* ============================================================== */

#ifdef HAVE_GUILE2
 #define C(X) ((scm_t_subr) X)
#else
 #define C(X) ((SCM (*) ()) X)
#endif

void SchemeSmob::register_procs(void)
{
	scm_c_define_gsubr("cog-atom",              1, 0, 0, C(ss_atom));
	scm_c_define_gsubr("cog-handle",            1, 0, 0, C(ss_handle));
	scm_c_define_gsubr("cog-undefined-handle",  0, 0, 0, C(ss_undefined_handle));
	scm_c_define_gsubr("cog-new-node",          2, 0, 1, C(ss_new_node));
	scm_c_define_gsubr("cog-new-link",          1, 0, 1, C(ss_new_link));
	scm_c_define_gsubr("cog-node",              2, 0, 1, C(ss_node));
	scm_c_define_gsubr("cog-link",              1, 0, 1, C(ss_link));
	scm_c_define_gsubr("cog-delete",            1, 0, 1, C(ss_delete));
	scm_c_define_gsubr("cog-delete-recursive",  1, 0, 1, C(ss_delete_recursive));
	scm_c_define_gsubr("cog-purge",             1, 0, 1, C(ss_purge));
	scm_c_define_gsubr("cog-purge-recursive",   1, 0, 1, C(ss_purge_recursive));
	scm_c_define_gsubr("cog-atom?",             1, 0, 1, C(ss_atom_p));
	scm_c_define_gsubr("cog-node?",             1, 0, 1, C(ss_node_p));
	scm_c_define_gsubr("cog-link?",             1, 0, 1, C(ss_link_p));

	// property setters on atoms
	scm_c_define_gsubr("cog-set-av!",           2, 0, 0, C(ss_set_av));
	scm_c_define_gsubr("cog-set-tv!",           2, 0, 0, C(ss_set_tv));
	scm_c_define_gsubr("cog-inc-vlti!",         1, 0, 0, C(ss_inc_vlti));
	scm_c_define_gsubr("cog-dec-vlti!",         1, 0, 0, C(ss_dec_vlti));

	// property getters on atoms
	scm_c_define_gsubr("cog-name",              1, 0, 0, C(ss_name));
	scm_c_define_gsubr("cog-type",              1, 0, 0, C(ss_type));
	scm_c_define_gsubr("cog-arity",             1, 0, 0, C(ss_arity));
	scm_c_define_gsubr("cog-incoming-set",      1, 0, 0, C(ss_incoming_set));
	scm_c_define_gsubr("cog-outgoing-set",      1, 0, 0, C(ss_outgoing_set));
	scm_c_define_gsubr("cog-tv",                1, 0, 0, C(ss_tv));
	scm_c_define_gsubr("cog-av",                1, 0, 0, C(ss_av));

	// Truth-values
	scm_c_define_gsubr("cog-new-stv",           2, 0, 0, C(ss_new_stv));
	scm_c_define_gsubr("cog-new-ctv",           3, 0, 0, C(ss_new_ctv));
	scm_c_define_gsubr("cog-new-itv",           3, 0, 0, C(ss_new_itv));
	scm_c_define_gsubr("cog-tv?",               1, 0, 0, C(ss_tv_p));
	scm_c_define_gsubr("cog-stv?",              1, 0, 0, C(ss_stv_p));
	scm_c_define_gsubr("cog-ctv?",              1, 0, 0, C(ss_ctv_p));
	scm_c_define_gsubr("cog-itv?",              1, 0, 0, C(ss_itv_p));
	scm_c_define_gsubr("cog-tv->alist",         1, 0, 0, C(ss_tv_get_value));

	// Atom Spaces
	scm_c_define_gsubr("cog-new-atomspace",     0, 1, 0, C(ss_new_as));
	scm_c_define_gsubr("cog-atomspace?",        1, 0, 0, C(ss_as_p));
	scm_c_define_gsubr("cog-atomspace",         0, 0, 0, C(ss_get_as));
	scm_c_define_gsubr("cog-set-atomspace!",    1, 0, 0, C(ss_set_as));

	// Attention values
	scm_c_define_gsubr("cog-new-av",            3, 0, 0, C(ss_new_av));
	scm_c_define_gsubr("cog-av?",               1, 0, 0, C(ss_av_p));
	scm_c_define_gsubr("cog-av->alist",         1, 0, 0, C(ss_av_get_value));

	// AttentionalFocus
	scm_c_define_gsubr("cog-af-boundary",       0, 0, 0, C(ss_af_boundary));
	scm_c_define_gsubr("cog-set-af-boundary!",  1, 0, 0, C(ss_set_af_boundary));
	scm_c_define_gsubr("cog-af",                0, 0, 0, C(ss_af));
    
	// ExecutionLinks
	scm_c_define_gsubr("cog-execute!",          1, 0, 0, C(ss_execute));

	// Atom types
	scm_c_define_gsubr("cog-get-types",         0, 0, 0, C(ss_get_types));
	scm_c_define_gsubr("cog-type?",             1, 0, 0, C(ss_type_p));
	scm_c_define_gsubr("cog-get-subtypes",      1, 0, 0, C(ss_get_subtypes));
	scm_c_define_gsubr("cog-subtype?",          2, 0, 0, C(ss_subtype_p));

	// Iterators
	scm_c_define_gsubr("cog-map-type",          2, 0, 0, C(ss_map_type));
}

#endif
/* ===================== END OF FILE ============================ */
