/*
 * verkko.h
 *
 *  Created on: 14.2.2009
 *      Author: Administrator
 */

#ifndef VERKKO_H_
#define VERKKO_H_

/*
 * Structit
 */
struct snmp SR_2;	//SET-REQUEST, oid 2

struct snmp GR_4;	//GET-REQUEST, oid 4, salvoon vastaus


/*
 * SNMP-viesti structit
 */
struct viesti {
	u_long osoite;
    u_short portti;
    u_short daatankoko;
    u_char daatta[127];		//payload data
    //int data_alkaa_indeksi;
    //int data_length;
};

/* SNMP viestin purkaminen */
struct snmp_message{
	u_char message_type;
	u_char message_length;
};

struct snmp_version{
	u_char version_type;
	u_char version_length;
	u_char version_value;
};

struct snmp_comm_string{
	u_char comm_string_type;
	u_char comm_string_length;
	u_char comm_string_value[10];  //Koska comm_string "Battleship" = 10
};

struct snmp_PDU{
	u_char PDU_type;
	u_char PDU_length;
};

struct snmp_requestID{
	u_char requestID_type;
	u_char requestID_length;
	//u_char requestID_value[2];	19.3.2009
	u_char requestID_value[5];	//Riittääkö 4???
};

struct snmp_error{
	u_char error_type;
	u_char error_length;
	u_char error_value;
};

struct snmp_error_index{
	u_char error_index_type;
	u_char error_index_length;
	u_char error_index_value;
};

struct snmp_varbind_list{
	u_char varbind_list_type;
	u_char varbind_list_length;
};

struct snmp_varbind{
	u_char varbind_type;
	u_char varbind_length;
};

struct snmp_OID{
	u_char OID_type;
	u_char OID_length;
	u_char OID_value[7];
};

struct snmp_value{
	u_char value_type;
	u_char value_length;
	//u_short value_value[50];
	int value_value[50];
};


struct snmp{
	struct snmp_message message;
	struct snmp_version version;
	struct snmp_comm_string comm_string;
	struct snmp_PDU PDU;
	struct snmp_requestID requestID;
	struct snmp_error error;
	struct snmp_error_index error_index;
	struct snmp_varbind_list varbind_list;
	struct snmp_varbind varbind;
	struct snmp_OID OID;
	struct snmp_value value;
};

/*
 * Funktioesittelyt
 */
void initializeNetwork(void);

void receiveMessage(void);
void snmpReceive(void);
u_short checkOID(void);
void parseSnmpResponse(u_short oid, u_short *value);
void parseSnmpResponse_ok(struct snmp *lahteva ,u_short oid, u_short *value);

void parseOutboundMessage(u_char OIDlastValue, u_short value[]/*[50] 2.4.2009 */, u_short value_length);
void calculateLengths(struct snmp *laskettava);
void compileOutboundMessage(const struct snmp *lahteva, u_long osoite, u_short portti);
void sendMessageToChannel(struct viesti *lv);
void printStruct(char* nimi, const struct snmp *tulostettava);

u_short printLahetys(void);
void sendSnmpMessageTestaus (u_short choice);

#endif /* VERKKO_H_ */
