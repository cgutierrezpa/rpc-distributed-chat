/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include "store_service.h"

bool_t
xdr_response (XDR *xdrs, response *objp)
{
	//register int32_t *buf;

	 if (!xdr_string (xdrs, &objp->msg, MAX_SIZE))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->md5, MAX_MD5))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_store_1_argument (XDR *xdrs, store_1_argument *objp)
{
	 if (!xdr_string (xdrs, &objp->sender, MAX_SIZE))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->receiver, MAX_SIZE))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->msg_id))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->msg, MAX_SIZE))
		 return FALSE;
	 if (!xdr_string (xdrs, &objp->md5, MAX_MD5))
		 return FALSE;
	return TRUE;
}

bool_t
xdr_getmessage_1_argument (XDR *xdrs, getmessage_1_argument *objp)
{
	 if (!xdr_string (xdrs, &objp->user, MAX_SIZE))
		 return FALSE;
	 if (!xdr_u_int (xdrs, &objp->msg_id))
		 return FALSE;
	return TRUE;
}
