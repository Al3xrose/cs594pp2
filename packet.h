#include <string>
using namespace std;

class rdt_packet {
	private:
	string sequence_number;
	string packet_type;
	string payload;

	public:
	rdt_packet(string seq_num, string pack_type, string pay)
	{
	   sequence_number = seq_num;
	   packet_type = pack_type;
	   payload = pay;
	}

	rdt_packet(string strPacket)
	{
		sequence_number = string("");
		packet_type = string("");
		payload = string("");
		int i = 0;

		//Get sequence number	
		while(strPacket[i] != '|')
		{
			sequence_number += strPacket[i];
			i++;
		}

		//skip |
		i++;

		//Get packet type
		while(strPacket[i] != '|')
		{
			packet_type += strPacket[i];
			i++;
		}

		//skip |
		i++;

		//Get payload
		while(i < strPacket.length())
		{
			payload += strPacket[i];
			i++;
		}
	}

	string get_sequence_number()
	{
		return sequence_number;
	}

	string get_packet_type()
	{
		return packet_type;
	}

	string get_payload()
	{
		return payload;
	}

	void set_sequence_number(string seq_num)
	{
		sequence_number = seq_num;
	}

	void set_packet_type(string type)
	{
		packet_type = type;
	}

	void set_payload(string p)
	{
		payload = p;
	}

	string to_string()
	{
		string returnString = string();
		returnString += sequence_number;	
		returnString += '|';
		returnString += packet_type;
		returnString += '|';
		returnString += payload;
		return returnString;
	}
};

