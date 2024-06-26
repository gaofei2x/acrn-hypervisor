/*-
 * Copyright (c) 2013  Peter Grehan <grehan@freebsd.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 * $FreeBSD$
 */

/*
 * The block API to be used by acrn-dm block-device emulations. The routines
 * are thread safe, with no assumptions about the context of the completion
 * callback - it may occur in the caller's context, or asynchronously in
 * another thread.
 */

#ifndef _BLOCK_IF_H_
#define _BLOCK_IF_H_

#include <sys/uio.h>
#include <sys/unistd.h>

#include "iothread.h"

#define BLOCKIF_IOV_MAX		256	/* not practical to be IOV_MAX */

/*
 *  |<------------------------------------- bounced_size --------------------------------->|
 *  |<-------- alignment ------->|                            |<-------- alignment ------->|
 *  |<--- head --->|<------------------------ org_size ---------------------->|<-- tail -->|
 *  |              |             |                            |               |            |
 *  *--------------$-------------*----------- ... ------------*---------------$------------*
 *  |              |             |                            |               |            |
 *  |              start                                                      end          |
 *  aligned_dn_start                                          aligned_dn_end
 *  |__________head_area_________|                            |__________tail_area_________|
 *  |<--- head --->|             |                            |<-- end_rmd -->|<-- tail -->|
 *  |<-------- alignment ------->|                            |<-------- alignment ------->|
 *
 */
struct br_align_info {
	uint32_t	alignment;

	bool		is_iov_base_aligned;
	bool		is_iov_len_aligned;
	bool		is_offset_aligned;

	/*
	 * Needs to convert the misaligned request to an aligned one when
	 * O_DIRECT is used, but the request (either buffer address/length, or offset) is not aligned.
	 */
	bool		need_conversion;

	uint32_t	head;
	uint32_t	tail;
	uint32_t	org_size;
	uint32_t	bounced_size;

	off_t		aligned_dn_start;
	off_t		aligned_dn_end;

	/*
	 * A bounce_iov for aligned read/write access.
	 * bounce_iov.iov_base is aligned to @alignment
	 * bounce_iov.iov_len is @bounced_size (@head + @org_size + @tail)
	 */
	struct iovec	bounce_iov;
};

struct blockif_req {
	struct iovec		iov[BLOCKIF_IOV_MAX];
	int			iovcnt;
	off_t			offset;
	ssize_t			resid;
	void			(*callback)(struct blockif_req *req, int err);
	void			*param;
	int			qidx;

	struct br_align_info	align_info;
};

struct blockif_ctxt;
struct blockif_ctxt *blockif_open(const char *optstr, const char *ident, int queue_num,
	struct iothreads_info *iothrds_info);
off_t	blockif_size(struct blockif_ctxt *bc);
void	blockif_chs(struct blockif_ctxt *bc, uint16_t *c, uint8_t *h,
		    uint8_t *s);
int	blockif_sectsz(struct blockif_ctxt *bc);
void	blockif_psectsz(struct blockif_ctxt *bc, int *size, int *off);
int	blockif_queuesz(struct blockif_ctxt *bc);
int	blockif_is_ro(struct blockif_ctxt *bc);
int	blockif_candiscard(struct blockif_ctxt *bc);
int	blockif_read(struct blockif_ctxt *bc, struct blockif_req *breq);
int	blockif_write(struct blockif_ctxt *bc, struct blockif_req *breq);
int	blockif_flush(struct blockif_ctxt *bc, struct blockif_req *breq);
int	blockif_discard(struct blockif_ctxt *bc, struct blockif_req *breq);
int	blockif_cancel(struct blockif_ctxt *bc, struct blockif_req *breq);
int	blockif_close(struct blockif_ctxt *bc);
uint8_t	blockif_get_wce(struct blockif_ctxt *bc);
void	blockif_set_wce(struct blockif_ctxt *bc, uint8_t wce);
int	blockif_flush_all(struct blockif_ctxt *bc);
int	blockif_max_discard_sectors(struct blockif_ctxt *bc);
int	blockif_max_discard_seg(struct blockif_ctxt *bc);
int	blockif_discard_sector_alignment(struct blockif_ctxt *bc);

#endif /* _BLOCK_IF_H_ */
