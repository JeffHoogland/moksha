#include "pa.h"
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>

void
msg_recv_creds(Pulse *conn, Pulse_Tag *tag)
{
   int r;
   struct msghdr mh;
   struct iovec iov;
   union {
       struct cmsghdr hdr;
       uint8_t data[CMSG_SPACE(sizeof(struct ucred))];
   } cmsg;

   memset(&iov, 0, sizeof(iov));
   iov.iov_base = &tag->header[tag->pos];
   iov.iov_len = sizeof(tag->header) - tag->pos;

   memset(&cmsg, 0, sizeof(cmsg));
   memset(&mh, 0, sizeof(mh));
   mh.msg_iov = &iov;
   mh.msg_iovlen = 1;
   mh.msg_control = &cmsg;
   mh.msg_controllen = sizeof(cmsg);

   r = recvmsg(ecore_main_fd_handler_fd_get(conn->fdh), &mh, 0);
   if ((!r) || (r == sizeof(tag->header))) tag->auth = EINA_TRUE;
   else if (r < 0)
     {
        if (errno != EAGAIN) ecore_main_fd_handler_del(conn->fdh);
     }
   else
     {
        DBG("%zu bytes left", sizeof(tag->header) - r);
        tag->pos += r;
     }
}

Eina_Bool
msg_recv(Pulse *conn, Pulse_Tag *tag)
{
   long r;
   struct msghdr mh;
   struct iovec iov;
   union {
       struct cmsghdr hdr;
       uint8_t data[CMSG_SPACE(sizeof(struct ucred))];
   } cmsg;

   memset(&iov, 0, sizeof(iov));
   iov.iov_base = tag->data + tag->pos;
   iov.iov_len = tag->dsize - tag->pos;

   memset(&cmsg, 0, sizeof(cmsg));
   memset(&mh, 0, sizeof(mh));
   mh.msg_iov = &iov;
   mh.msg_iovlen = 1;
   mh.msg_control = &cmsg;
   mh.msg_controllen = sizeof(cmsg);

   r = recvmsg(ecore_main_fd_handler_fd_get(conn->fdh), &mh, 0);
   DBG("recv %li bytes", r);
   /* things we don't really care about: credentials */
   if ((!r) || ((unsigned int)r == tag->dsize))
     {
        conn->iq = eina_list_remove(conn->iq, tag);
        return EINA_TRUE;
     }
   else if (r < 0)
     {
        if (errno != EAGAIN) ecore_main_fd_handler_del(conn->fdh);
     }
   else
     tag->pos += r;
   return EINA_FALSE;
}

void
msg_sendmsg_creds(Pulse *conn, Pulse_Tag *tag)
{
   int r;
   struct msghdr mh;
   struct iovec iov;
   union {
       struct cmsghdr hdr;
       uint8_t data[CMSG_SPACE(sizeof(struct ucred))];
   } cmsg;
   struct ucred *u;

   memset(&iov, 0, sizeof(iov));
   iov.iov_base = (void*) &tag->header[tag->pos];
   iov.iov_len = sizeof(tag->header) - tag->pos;

   memset(&cmsg, 0, sizeof(cmsg));
   cmsg.hdr.cmsg_len = CMSG_LEN(sizeof(struct ucred));
   cmsg.hdr.cmsg_level = SOL_SOCKET;
   cmsg.hdr.cmsg_type = SCM_CREDENTIALS;

   u = (struct ucred*) CMSG_DATA(&cmsg.hdr);

   u->pid = getpid();
   u->uid = getuid();
   u->gid = getgid();

   memset(&mh, 0, sizeof(mh));
   mh.msg_iov = &iov;
   mh.msg_iovlen = 1;
   mh.msg_control = &cmsg;
   mh.msg_controllen = sizeof(cmsg);

   r = sendmsg(ecore_main_fd_handler_fd_get(conn->fdh), &mh, MSG_NOSIGNAL);
   if ((!r) || (r == (int)sizeof(tag->header))) tag->auth = EINA_TRUE;
   else if (r < 0)
     {
        if (errno != EAGAIN) ecore_main_fd_handler_del(conn->fdh);
     }
   else
     tag->pos += r;
}

void
msg_send_creds(Pulse *conn, Pulse_Tag *tag)
{
   int r;

   INF("trying to send 20 byte auth header");
   r = send(ecore_main_fd_handler_fd_get(conn->fdh), &tag->header[tag->pos], sizeof(tag->header) - tag->pos, MSG_NOSIGNAL);
   INF("%i bytes sent!", r);
   if ((!r) || (r == (int)sizeof(tag->header))) tag->auth = EINA_TRUE;
   else if (r < 0)
     {
        if (errno != EAGAIN) ecore_main_fd_handler_del(conn->fdh);
     }
   else
     tag->pos += r;
}

Eina_Bool
msg_send(Pulse *conn, Pulse_Tag *tag)
{
   int r;

   INF("trying to send %zu bytes", tag->dsize - tag->pos);
   r = send(ecore_main_fd_handler_fd_get(conn->fdh), tag->data + tag->pos, tag->dsize - tag->pos, MSG_NOSIGNAL);
   INF("%i bytes sent!", r);
   if ((!r) || ((unsigned int)r == tag->dsize - tag->pos))
     {
        DBG("Send complete! Deleting tag...");
        conn->oq = eina_list_remove(conn->oq, tag);
        pulse_tag_free(tag);
        return EINA_TRUE;
     }
   if (r < 0)
     {
        if (errno != EAGAIN) ecore_main_fd_handler_del(conn->fdh);
     }
   else
     tag->pos += r;
   return EINA_FALSE;
}
