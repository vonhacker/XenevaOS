/**
* BSD 2-Clause License
*
* Copyright (c) 2022-2023, Manas Kamal Choudhury
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* 1. Redistributions of source code must retain the above copyright notice, this
*    list of conditions and the following disclaimer.
*
* 2. Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
* FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
* DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
* SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
* CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
* OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
* OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
**/

#include <Net\ethernet.h>
#include <Net\aunet.h>
#include <Mm\kmalloc.h>
#include <string.h>
#include <Hal\serial.h>
#include <Fs\Dev\devfs.h>
#include <aucon.h>
#include <Net\ipv4.h>
#include <Net\socket.h>

#pragma pack(push,1)
__declspec(align(2)) typedef struct _ethernet_ {
	uint8_t dest[6];
	uint8_t src[6];
	uint16_t typeLen;
	uint8_t payload[];
}Ethernet;
#pragma pack(pop)

AU_EXTERN AU_EXPORT void AuEthernetHandle(void *data, int size) {
	Ethernet* frame = (Ethernet*)data;
	list_t *raw_sockets = AuRawSocketGetList();
	for (int i = 0; i < raw_sockets->pointer; i++) {
		AuSocket *sock = (AuSocket*)list_get_at(raw_sockets, i);
		AuSocketAdd(sock, frame, size);
	}
	switch (ntohs(frame->typeLen)) {
	case ETHERNET_TYPE_ARP:
		SeTextOut("ARP Packet received \r\n");
		break;
	case ETHERNET_TYPE_IPV4:
		IPv4HandlePacket((void*)&frame->payload);
		break;
	}
}

/*
 * AuEthernetSend -- sends a packet to ethernet layer
 * @param data -- data to send
 * @param len -- length of the data
 * @param type -- type
 * @param dest -- destination mac address
 */
void AuEthernetSend(AuVFSNode* nic,void* data, size_t len, uint16_t type, uint8_t* dest) {
	AuNetworkDevice* ndev = (AuNetworkDevice*)nic->device;
	if (!ndev)
		return;
	size_t totalSz = sizeof(Ethernet)+len;
	Ethernet* pacl = (Ethernet*)kmalloc(totalSz);
	memset(pacl, 0, totalSz);
	memcpy(pacl->dest, dest, 6);
	uint8_t *src_mac = ndev->mac;
	memcpy(pacl->src, src_mac, 6);
	pacl->typeLen = htons(type);
	memcpy(pacl->payload, data, len);
	
	AuTextOut("Ethernet setuped %d \r\n", ndev->type);

	if (nic->write)
		nic->write(nic, nic, (uint64_t*)pacl, totalSz);

	kfree(pacl);
}