/**
 * @file				BleLinkConnect.h
 * @brief			BleLinkConnect.c ��ͷ�ļ�
 * @author			fengxun
 * @date			2017年12月26日
*/
#ifndef __BLELINKCONNECT_H__
#define __BLELINKCONNECT_H__
#include "../../DhGlobalHead.h"

typedef struct
{
    u1  m_u1PeerBleAddrType;
    u1  m_pu1PeerBleAddr[BLE_ADDR_LEN];
}BlkBleConnInfo;



#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* __cplusplus */

extern void LinkConnReqHandle(u1 addrType, u1 *pu1Addr, u1* pu1LLData);
extern void LinkConnStateInit(u1 sca);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __BLELINKCONNECT_H__ */
