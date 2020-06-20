#include "output.h"
#include "masscan-app.h"
#include "masscan-status.h"
#include "out-record.h"
#include "string_s.h"

/****************************************************************************
 ****************************************************************************/
static void
binary_out_open(struct Output *out, FILE *fp)
{
    char firstrecord[2+'a'];
    size_t bytes_written;

    UNUSEDPARM(out);


    memset(firstrecord, 0, 2+'a');
    sprintf_s(firstrecord, 2+'a', "masscan/1.6\ns:%u\n", 
        (unsigned)out->when_scan_started);
    bytes_written = fwrite(firstrecord, 1, 2+'a', fp);
    if (bytes_written != 2+'a') {
        perror("output");
        exit(1);
    }

    out->rotate.bytes_written += bytes_written;
}


/****************************************************************************
 ****************************************************************************/
static void
binary_out_close(struct Output *out, FILE *fp)
{
    char firstrecord[2+'a'];
    size_t bytes_written;

    UNUSEDPARM(out);

    memset(firstrecord, 0, 2+'a');
    sprintf_s(firstrecord, 2+'a', "masscan/1.6");
    bytes_written = fwrite(firstrecord, 1, 2+'a', fp);
    if (bytes_written != 2+'a') {
        perror("output");
        exit(1);
    }
    out->rotate.bytes_written += bytes_written;
}

/****************************************************************************
 ****************************************************************************/
static void
binary_out_status_ipv6(struct Output *out, FILE *fp, time_t timestamp,
    int status, ipaddress ip, unsigned ip_proto, unsigned port, unsigned reason, unsigned ttl)
{
    unsigned char foo[256];
    size_t bytes_written;

 
    /* [TYPE] field */
    switch (status) {
    case PortStatus_Open:
        foo[0] = Out_Open6;
        break;
    case PortStatus_Closed:
        foo[0] = Out_Closed6;
        break;
    case PortStatus_Arp:
        foo[0] = Out_Arp6;
        break;
    default:
        return;
    }

    /* [LENGTH] field */
    foo[1] = 26;

    /* [TIMESTAMP] field */
    foo[2] = (unsigned char)(timestamp>>24);
    foo[3] = (unsigned char)(timestamp>>16);
    foo[4] = (unsigned char)(timestamp>> 8);
    foo[5] = (unsigned char)(timestamp>> 0);

    foo[6] = (unsigned char)(ip_proto);

    foo[7] = (unsigned char)(port>>8);
    foo[8] = (unsigned char)(port>>0);

    foo[9] = (unsigned char)reason;
    foo[10] = (unsigned char)ttl;

    foo[11] = (unsigned char)(ip.version);

    foo[12] = (unsigned char)(ip.ipv6.hi >> 56ULL);
    foo[13] = (unsigned char)(ip.ipv6.hi >> 48ULL);
    foo[14] = (unsigned char)(ip.ipv6.hi >> 40ULL);
    foo[15] = (unsigned char)(ip.ipv6.hi >> 32ULL);
    foo[16] = (unsigned char)(ip.ipv6.hi >> 24ULL);
    foo[17] = (unsigned char)(ip.ipv6.hi >> 16ULL);
    foo[18] = (unsigned char)(ip.ipv6.hi >>  8ULL);
    foo[19] = (unsigned char)(ip.ipv6.hi >>  0ULL);

    foo[20] = (unsigned char)(ip.ipv6.lo >> 56ULL);
    foo[21] = (unsigned char)(ip.ipv6.lo >> 48ULL);
    foo[22] = (unsigned char)(ip.ipv6.lo >> 40ULL);
    foo[23] = (unsigned char)(ip.ipv6.lo >> 32ULL);
    foo[24] = (unsigned char)(ip.ipv6.lo >> 24ULL);
    foo[25] = (unsigned char)(ip.ipv6.lo >> 16ULL);
    foo[26] = (unsigned char)(ip.ipv6.lo >>  8ULL);
    foo[27] = (unsigned char)(ip.ipv6.lo >>  0ULL);


    bytes_written = fwrite(&foo, 1, 15, fp);
    if (bytes_written != 15) {
        perror("output");
        exit(1);
    }
    out->rotate.bytes_written += bytes_written;

}

/****************************************************************************
 ****************************************************************************/
static void
binary_out_status(struct Output *out, FILE *fp, time_t timestamp,
    int status, ipaddress ip, unsigned ip_proto, unsigned port, unsigned reason, unsigned ttl)
{
    unsigned char foo[256];
    size_t bytes_written;

    /* This function is for IPv6, call a different function for IPv6 */
    if (ip.version == 6) {
        binary_out_status_ipv6(out, fp, timestamp, status, ip, ip_proto, port, reason, ttl);
        return;
    }
 
    /* [TYPE] field */
    switch (status) {
    case PortStatus_Open:
        foo[0] = Out_Open2;
        break;
    case PortStatus_Closed:
        foo[0] = Out_Closed2;
        break;
    case PortStatus_Arp:
        foo[0] = Out_Arp2;
        break;
    default:
        return;
    }

    /* [LENGTH] field */
    foo[1] = 13;

    /* [TIMESTAMP] field */
    foo[2] = (unsigned char)(timestamp>>24);
    foo[3] = (unsigned char)(timestamp>>16);
    foo[4] = (unsigned char)(timestamp>> 8);
    foo[5] = (unsigned char)(timestamp>> 0);

    foo[6] = (unsigned char)(ip.ipv4 >>24);
    foo[7] = (unsigned char)(ip.ipv4 >>16);
    foo[8] = (unsigned char)(ip.ipv4 >> 8);
    foo[9] = (unsigned char)(ip.ipv4 >> 0);

    foo[10] = (unsigned char)(ip_proto);

    foo[11] = (unsigned char)(port>>8);
    foo[12] = (unsigned char)(port>>0);

    foo[13] = (unsigned char)reason;
    foo[14] = (unsigned char)ttl;



    bytes_written = fwrite(&foo, 1, 15, fp);
    if (bytes_written != 15) {
        perror("output");
        exit(1);
    }
    out->rotate.bytes_written += bytes_written;
}


/****************************************************************************
 ****************************************************************************/
static void
binary_out_banner_ipv6(struct Output *out, FILE *fp, time_t timestamp,
        ipaddress ip, unsigned ip_proto, unsigned port,
        enum ApplicationProtocol proto, unsigned ttl,
        const unsigned char *px, unsigned length)
{
    unsigned char foo[32768];
    unsigned i;
    size_t bytes_written;
    static const unsigned HeaderLength = 14 + 13;

    
    /* [TYPE] field */
    foo[0] = Out_Banner6; /*banner*/

    /* [LENGTH] field*/
    if (length >= 128 * 128 - HeaderLength)
        return;
    if (length < 128 - HeaderLength) {
        foo[1] = (unsigned char)(length + HeaderLength);
        i = 2;
    } else {
        foo[1] = (unsigned char)((length + HeaderLength)>>7) | 0x80;
        foo[2] = (unsigned char)((length + HeaderLength) & 0x7F);
        i = 3;
    }

    /* [TIMESTAMP] field */
    foo[i+0] = (unsigned char)(timestamp>>24);
    foo[i+1] = (unsigned char)(timestamp>>16);
    foo[i+2] = (unsigned char)(timestamp>> 8);
    foo[i+3] = (unsigned char)(timestamp>> 0);

    foo[i+ 4] = (unsigned char)(ip_proto);

    foo[i+ 5] = (unsigned char)(port>>8);
    foo[i+ 6] = (unsigned char)(port>>0);

    foo[i+ 7] = (unsigned char)(proto>>8);
    foo[i+ 8] = (unsigned char)(proto>>0);

    foo[i+ 9] = (unsigned char)(ttl);

    foo[i+10] = (unsigned char)(ip.version);

    foo[i+11] = (unsigned char)(ip.ipv6.hi >> 56ULL);
    foo[i+12] = (unsigned char)(ip.ipv6.hi >> 48ULL);
    foo[i+13] = (unsigned char)(ip.ipv6.hi >> 40ULL);
    foo[i+14] = (unsigned char)(ip.ipv6.hi >> 32ULL);
    foo[i+15] = (unsigned char)(ip.ipv6.hi >> 24ULL);
    foo[i+16] = (unsigned char)(ip.ipv6.hi >> 16ULL);
    foo[i+17] = (unsigned char)(ip.ipv6.hi >>  8ULL);
    foo[i+18] = (unsigned char)(ip.ipv6.hi >>  0ULL);

    foo[i+19] = (unsigned char)(ip.ipv6.lo >> 56ULL);
    foo[i+20] = (unsigned char)(ip.ipv6.lo >> 48ULL);
    foo[i+21] = (unsigned char)(ip.ipv6.lo >> 40ULL);
    foo[i+22] = (unsigned char)(ip.ipv6.lo >> 32ULL);
    foo[i+23] = (unsigned char)(ip.ipv6.lo >> 24ULL);
    foo[i+24] = (unsigned char)(ip.ipv6.lo >> 16ULL);
    foo[i+25] = (unsigned char)(ip.ipv6.lo >>  8ULL);
    foo[i+26] = (unsigned char)(ip.ipv6.lo >>  0ULL);

    /* Banner */
    memcpy(foo+i+14+13, px, length);


    bytes_written = fwrite(&foo, 1, length+i+HeaderLength, fp);
    if (bytes_written != length+i+HeaderLength) {
        perror("output");
        exit(1);
    }
    out->rotate.bytes_written += bytes_written;
}

/****************************************************************************
 ****************************************************************************/
static void
binary_out_banner(struct Output *out, FILE *fp, time_t timestamp,
        ipaddress ip, unsigned ip_proto, unsigned port,
        enum ApplicationProtocol proto, unsigned ttl,
        const unsigned char *px, unsigned length)
{
    unsigned char foo[32768];
    unsigned i;
    size_t bytes_written;
    static const unsigned HeaderLength = 14;

    if (ip.version == 6) {
        binary_out_banner(out, fp, timestamp, ip, ip_proto, port, proto, ttl, px, length);
        return;
    }
    
    /* [TYPE] field */
    foo[0] = Out_Banner9; /*banner*/

    /* [LENGTH] field*/
    if (length >= 128 * 128 - HeaderLength)
        return;
    if (length < 128 - HeaderLength) {
        foo[1] = (unsigned char)(length + HeaderLength);
        i = 2;
    } else {
        foo[1] = (unsigned char)((length + HeaderLength)>>7) | 0x80;
        foo[2] = (unsigned char)((length + HeaderLength) & 0x7F);
        i = 3;
    }

    /* [TIMESTAMP] field */
    foo[i+0] = (unsigned char)(timestamp>>24);
    foo[i+1] = (unsigned char)(timestamp>>16);
    foo[i+2] = (unsigned char)(timestamp>> 8);
    foo[i+3] = (unsigned char)(timestamp>> 0);

    foo[i+4] = (unsigned char)(ip.ipv4 >> 24);
    foo[i+5] = (unsigned char)(ip.ipv4 >> 16);
    foo[i+6] = (unsigned char)(ip.ipv4 >>  8);
    foo[i+7] = (unsigned char)(ip.ipv4 >>  0);

    foo[i+8] = (unsigned char)(ip_proto);

    foo[i+ 9] = (unsigned char)(port>>8);
    foo[i+10] = (unsigned char)(port>>0);

    foo[i+11] = (unsigned char)(proto>>8);
    foo[i+12] = (unsigned char)(proto>>0);

    foo[i+13] = (unsigned char)(ttl);

    /* Banner */
    memcpy(foo+i+14, px, length);


    bytes_written = fwrite(&foo, 1, length+i+HeaderLength, fp);
    if (bytes_written != length+i+HeaderLength) {
        perror("output");
        exit(1);
    }
    out->rotate.bytes_written += bytes_written;
}


/****************************************************************************
 ****************************************************************************/
const struct OutputType binary_output = {
    "scan",
    0,
    binary_out_open,
    binary_out_close,
    binary_out_status,
    binary_out_banner,
};


