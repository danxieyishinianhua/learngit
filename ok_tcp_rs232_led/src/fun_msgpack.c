#include "fun_msgpack.h"
#include "nodearray.h"
#include "send_list.h"
#include "tcp_rs232.h"

msgpack_sbuffer* g_pk_buffer = NULL;
msgpack_packer* g_pk = NULL;
msgpack_unpacker* g_unpk = NULL;

uint8 tcp_num_rigester = 0;//tcp注册次数
uint8 resend_cmd_flag = 0;//重发命令标志位
long  recv_cmd_time;//接受命令的时间
uint8 resend_num = 0;//重发的次数

void fun_msgpack_init(void)
{
	g_pk_buffer = msgpack_sbuffer_new();
    g_pk = msgpack_packer_new(g_pk_buffer, msgpack_sbuffer_write);
	g_unpk = msgpack_unpacker_new(BUF_MSGPACK);
}


int read_mac(char *hwaddr)
{
	char mac[20] = {0};
	int fd = open("/mnt/mac.log", O_RDWR | O_SYNC);
	if(fd < 0) {
        printf("Could not open mac.log\n");
        return -1;
    }
	else
	{
		char macbuf[4096];
		memset(macbuf, 0, sizeof(macbuf));
		int size = read(fd, macbuf, sizeof(macbuf));
		if(size > 0){
			char *str = NULL, *str1;
			if((str = strstr(macbuf, "eth1")) != NULL){
				if((str1 = strstr(str, "HWaddr ")) != NULL){
					memcpy(mac, str1 + 7, 17);
					memcpy(hwaddr, mac, 17);
					printf("AR9331 mac:%s\n", mac);
					return 1;
				}
			}
		}
	}
	return -2;
}

//该函数用于响应网关是否连接成功
void msg_LcpLinkInitRsp(msgpack_object* result)
{
	msgpack_object* err_code_obj = result->via.array.ptr + 1;
	int responsscode = err_code_obj->via.u64;
	if(0 != responsscode)//响应码值为0就连接上服务器了，否则未连接上
	{
		zigbee_tcp_connect = 0;//zigbee未连接
	}
	else
	{
		zigbee_tcp_connect = 2;//标志置为2说明zigbee登录成功	
	}
	return;
}

void msg_LcpLampLoginRsp(msgpack_object* result)
{
	printf("******msg_LcpLampLoginRsp******\n");
	msgpack_object* err_code_obj = result->via.array.ptr + 1;
	int responsscode = err_code_obj->via.u64;
	if(0 == responsscode)
	{
		check_login_flag(1);	
	}
	return;
}

void msg_LcpPadLoginRsp(msgpack_object* result)
{
	printf("******msg_LcpPadLoginRsp******\n");
	msgpack_object* err_code_obj = result->via.array.ptr + 1;
	int responsscode = err_code_obj->via.u64;
	if(0 == responsscode)
	{
		check_login_flag(0);//面板就是控制器，如果是面板或红外传感器，节点的登录标志位置为3
	}
	return;
}



void msg_LCP_LINK_KEEPALIVE_RSP(msgpack_object* result)
{
	msgpack_object* err_code_obj = result->via.array.ptr + 1;
	int responsscode = err_code_obj->via.u64;
	if(0 == responsscode)
	{
		tcp_num_rigester = 0;//连接上了就置为0
	}
	return;
}

void msg_LcpSetBrightnessColorTemp(msgpack_object* result)
{
	msgpack_object* LampId_obj = result->via.array.ptr + 1;
	msgpack_object* Brightness_obj = result->via.array.ptr + 2;
	msgpack_object* ColorTemp_obj  = result->via.array.ptr + 3;

	char mac_temp[20] = {0};
	int str_len, ColorTemp, Brightness;
	int erro = 0;
	str_len = LampId_obj->via.str.size;
	if(str_len == MAC_LEN)memcpy(mac_temp, LampId_obj->via.str.ptr, MAC_LEN);
	else erro = 1;
	Brightness = Brightness_obj->via.i64;
	ColorTemp  = ColorTemp_obj->via.i64;

	//接受消息处理，并加入串口发送队列
	unsigned char send_buf[20];
	send_buf[0] = HEADFLAG;
	send_buf[1] = 0x0A;
	send_buf[3] = 0x01;
	send_buf[9] = ENDFLAG;
	get_Mac_Hex(&mac_temp[MAC_offset], (unsigned char *)&send_buf[4], MAC_LEN - MAC_offset);
	if(Brightness <= -1)//单调色温
	{
		if(ColorTemp > 255)ColorTemp = 255, erro = 2;
		send_buf[2] = MSG_SET_COLORTEMP;
		send_buf[8] = ColorTemp;
		add_zbnode_light_parm((unsigned char *)&send_buf[4], 0, ColorTemp, 0, 0, 0, COLORTEMP);
	}	
	else if(ColorTemp <= -1)//单调亮度
	{
		if(Brightness > 100)Brightness = 100, erro = 3;
		send_buf[2] = MSG_SET_LIGHTNESS;
		send_buf[8] = Brightness;
		add_zbnode_light_parm((unsigned char *)&send_buf[4], Brightness, 0, 0, 0, 0, BRIGHTNESS);
	}
	else//亮度和色温同时调节
	{	
		if(ColorTemp > 255)ColorTemp = 255, erro = 2;
		if(Brightness > 100)Brightness = 100, erro = 3;
		send_buf[1] = 0x0D;//消息长度
		send_buf[2] = MSG_SET_LAMPS;//灯具灯光请求(场景)
		send_buf[8] = Brightness;//亮度值
		send_buf[9] = ColorTemp;//色温值
		send_buf[10] = 0;
		send_buf[11] = 0;//10-11保留未用
		send_buf[12] = ENDFLAG;//消息尾
		add_zbnode_light_parm((unsigned char *)&send_buf[4], Brightness, ColorTemp, 0, 0, 0, BRIGHTNESS | COLORTEMP);
	}
	if(erro == 0)
	{
		msg_write_to_list((char *)&send_buf, send_buf[1]);
		resend_cmd_flag = 1;
	}

	recv_cmd_time = getcurrenttime();
    resend_num = 0;
	//	tcp消息响应
	msgpack_sbuffer_clear(g_pk_buffer);
	msgpack_pack_array(g_pk, 5);
	msgpack_pack_uint32(g_pk, LCP_SET_BRIGHTNESS_COLOR_TEMP_RSP);
	msgpack_pack_str(g_pk, MAC_LEN); 
	msgpack_pack_str_body(g_pk, mac_temp, MAC_LEN);
	msgpack_pack_uint32(g_pk, erro);
	msgpack_pack_uint32(g_pk, Brightness);
	msgpack_pack_uint32(g_pk, ColorTemp);

	tcp_send(sockfd_server, (void *)g_pk_buffer->data, g_pk_buffer->size);
	return;
}

void msg_LcpSetRGB(msgpack_object* result)
{
	msgpack_object* LampId_obj     = result->via.array.ptr + 1;
	msgpack_object* Brightness_obj = result->via.array.ptr + 2;
	msgpack_object* R_val_obj 	   = result->via.array.ptr + 3;
	msgpack_object* G_val_obj      = result->via.array.ptr + 4;
	msgpack_object* B_val_obj      = result->via.array.ptr + 5;
	

	char mac_temp[20] = {0};
	int str_len, Brightness, r_val, g_val, b_val;
	int erro = 1;
	str_len = LampId_obj->via.str.size;//获得该数组的大小
	
	if(str_len == MAC_LEN)memcpy(mac_temp, LampId_obj->via.str.ptr, MAC_LEN);
	else erro = 1;
	Brightness = Brightness_obj->via.i64;//暂时未用
	r_val = R_val_obj->via.i64;
	g_val = G_val_obj->via.i64;
	b_val = B_val_obj->via.i64;
	if(r_val > 255 || r_val < 0);
	else if(g_val > 255 || g_val < 0);
	else if(b_val > 255 || b_val < 0);
	else erro = 0;

	//接受消息处理，并加入串口发送队列
	unsigned char send_buf[20];
	send_buf[0] = HEADFLAG;
	send_buf[1] = 0x0C;
	send_buf[2] = MSG_SET_RGB;
	send_buf[3] = 0x01;
	get_Mac_Hex(&mac_temp[MAC_offset], (unsigned char *)&send_buf[4], MAC_LEN - MAC_offset);
	send_buf[8] = r_val;
	send_buf[9] = g_val;
	send_buf[10] = b_val;
	send_buf[11] = ENDFLAG;
	if(erro == 0)
	{
		msg_write_to_list((char *)&send_buf, send_buf[1]);
		add_zbnode_light_parm((unsigned char *)&send_buf[4], 0, 0, r_val, g_val, b_val, R_G_B);
		resend_cmd_flag = 1;//重新发送命令标志位置1
	}

	//	tcp消息响应
	msgpack_sbuffer_clear(g_pk_buffer);
	msgpack_pack_array(g_pk, 7);
	msgpack_pack_uint32(g_pk, LCP_SET_RGB_RSP);
	msgpack_pack_str(g_pk, MAC_LEN); 
	msgpack_pack_str_body(g_pk, mac_temp, MAC_LEN);
	msgpack_pack_uint32(g_pk, erro);
	msgpack_pack_uint32(g_pk, Brightness);
	msgpack_pack_uint32(g_pk, r_val);
	msgpack_pack_uint32(g_pk, g_val);
	msgpack_pack_uint32(g_pk, b_val);

	recv_cmd_time = getcurrenttime();
	resend_num = 0;//重发次数清零

	tcp_send(sockfd_server,(void *)g_pk_buffer->data, g_pk_buffer->size);
	return;
}

//该函数用于服务器获取灯的信息
void msg_LcpGetLampInfo (msgpack_object* result)
{
	msgpack_object* LampId_obj = result->via.array.ptr + 1;
	
	char mac_temp[20] = {0};
	int str_len = 0;
	int ret = 0;
	str_len = LampId_obj->via.str.size;
	
	if(str_len == MAC_LEN)memcpy(mac_temp, LampId_obj->via.str.ptr, MAC_LEN);
	else return;

	//接受消息处理
	
	struct LampInfo LampInfotemp;
	get_Mac_Hex(mac_temp, (unsigned char *)&LampInfotemp.mac[2], MAC_LEN);
	//发送给服务器的mac地址需要后面6个字节，而辨别具体是那盏灯只需后面四个字节
	LampInfotemp.mac[0] = 0x0;
	LampInfotemp.mac[1] = 0x0D;
	ret = read_lamp_info(LampInfotemp);
	
	//	tcp消息响应
	if(ret)
	{
		//先分两组，再分7组-->服务器需要查询多个灯的状态，先比较msg_id是否正确，正确在分7组用来存放灯的信息，直接分8组只能用于查询一个灯的信息
		msgpack_sbuffer_clear(g_pk_buffer);
		msgpack_pack_array(g_pk, 2);
		msgpack_pack_uint32(g_pk, LCP_GET_LAMP_INFO_RSP);
		
		msgpack_pack_array(g_pk, 7);
		msgpack_pack_str(g_pk, MAC_LEN); 
		msgpack_pack_str_body(g_pk, mac_temp, MAC_LEN);
		msgpack_pack_uint32(g_pk, LampInfotemp.brightness);
		msgpack_pack_uint32(g_pk, LampInfotemp.colortemp);
		msgpack_pack_uint32(g_pk, LampInfotemp.lightness);
		msgpack_pack_uint32(g_pk, LampInfotemp.valu_r);
		msgpack_pack_uint32(g_pk, LampInfotemp.valu_g); 
		msgpack_pack_uint32(g_pk, LampInfotemp.valu_b);

		tcp_send(sockfd_server, (void *)g_pk_buffer->data, g_pk_buffer->size);
	}
	return;
}

void msg_LcpSetGroupLampInfo(msgpack_object* result)
{

	msgpack_object* Lamps_Id_obj      = result->via.array.ptr + 1;
	msgpack_object* Brightness_obj    = result->via.array.ptr + 2;
	msgpack_object* ColorTemp_obj     = result->via.array.ptr + 3;
	msgpack_object* RgbBrightness_obj = result->via.array.ptr + 4;
	msgpack_object* R_val_obj         = result->via.array.ptr + 5;
	msgpack_object* G_val_obj         = result->via.array.ptr + 6;
	msgpack_object* B_val_obj         = result->via.array.ptr + 7;
	
	//接受消息
	char mac_temp[20] = {0};
	unsigned char macs_temp[MAX_UART_SEND_LEN] = {0};
	uint8 str_len, Brightness, ColorTemp, r_val, g_val, b_val;
	int erro = 1, lamp_num = 0, i, copy_mac_num = 0;
	lamp_num = Lamps_Id_obj->via.array.size;
	printf("msg_LcpSetGroupLampInfo lamp_num:%d\n", lamp_num);
	if(lamp_num > 100)lamp_num = 100;
	uint8 set_light_flag;//设置亮度标志位

	
	if(Brightness_obj->via.i64 != -1)set_light_flag = BRIGHTNESS;
	else if(ColorTemp_obj->via.i64 != -1)set_light_flag = COLORTEMP;
	else set_light_flag = R_G_B;
	Brightness = (uint8)(Brightness_obj->via.i64);
	ColorTemp = (uint8)(ColorTemp_obj->via.i64);
	//rgb_li = (uint8)(RgbBrightness_obj->via.i64);
	r_val = (uint8)(R_val_obj->via.i64);
	g_val = (uint8)(G_val_obj->via.i64);
	b_val = (uint8)(B_val_obj->via.i64);
	
    #ifdef TCP_RECV_DEBUG
	int a, b, c, d, e;
	a = (Brightness_obj->via.i64);
	b = (ColorTemp_obj->via.i64);
	c = (R_val_obj->via.i64);
	d = (G_val_obj->via.i64);
	e = (B_val_obj->via.i64);
	printf("Brightness:%d ColorTemp:%d  r_val:%d  g_val:%d  b_val:%d\n", a, b, c, d, e);
    #endif

	unsigned char send_buf[MAX_UART_SEND_LEN] = {0};//90
	send_buf[0] = HEADFLAG;
    msgpack_object* LampId_obj = NULL;
	for(i = 0; i < lamp_num; i++)
	{
		LampId_obj = Lamps_Id_obj->via.array.ptr + i;
		str_len = LampId_obj->via.str.size;
		if(str_len == MAC_LEN)
		{
			memcpy(mac_temp, LampId_obj->via.str.ptr, MAC_LEN);
			get_Mac_Hex(&mac_temp[MAC_offset], (unsigned char *)&macs_temp[4*copy_mac_num], MAC_LEN - MAC_offset);
			add_zbnode_light_parm((char *)&macs_temp[4*copy_mac_num], Brightness, ColorTemp, r_val, g_val, b_val, set_light_flag);
			copy_mac_num++;
			if(copy_mac_num >= MAX_LAMP_SEND_LIGHT)//20
			{
				send_buf[1] = copy_mac_num * 4 + 6;
				send_buf[3] = copy_mac_num;//灯的个数
				memcpy((unsigned char *)&send_buf[4], macs_temp, copy_mac_num * 4);
				send_buf[copy_mac_num * 4 + 5] = ENDFLAG;
				if(set_light_flag == BRIGHTNESS){send_buf[2] = MSG_SET_LIGHTNESS; send_buf[copy_mac_num * 4 + 4] = Brightness;}
				else if(set_light_flag == COLORTEMP){send_buf[2] = MSG_SET_COLORTEMP; send_buf[copy_mac_num * 4 + 4] = ColorTemp;}
				else //设置RGB的消息长度与设置亮度与色温不一样
				{
					send_buf[2] = MSG_SET_RGB;
					send_buf[1] = copy_mac_num*4 + 8;//重新设置消息长度
					send_buf[copy_mac_num*4 + 4] = r_val;
					send_buf[copy_mac_num*4 + 5] = g_val;
					send_buf[copy_mac_num*4 + 6] = b_val;
					send_buf[copy_mac_num*4 + 7] = ENDFLAG;
				}
				msg_write_to_list((char *)&send_buf, send_buf[1]);
				copy_mac_num = 0;
				resend_cmd_flag = 1;//重发命令标志位置1
			}
		}
		else erro = 1;
	}	

	if(copy_mac_num != 0)
	{
		send_buf[1] = copy_mac_num*4 + 6;
		send_buf[3] = copy_mac_num;
		memcpy((unsigned char *)&send_buf[4], macs_temp, copy_mac_num*4);
		send_buf[copy_mac_num*4 + 5] = ENDFLAG;
		if(set_light_flag == BRIGHTNESS){send_buf[2] = MSG_SET_LIGHTNESS; send_buf[copy_mac_num * 4 + 4] = Brightness;}
		else if(set_light_flag == COLORTEMP){send_buf[2] = MSG_SET_COLORTEMP; send_buf[copy_mac_num * 4 + 4] = ColorTemp;}
		else 
		{
			send_buf[2] = MSG_SET_RGB;
			send_buf[1] = copy_mac_num * 4 + 8;
			send_buf[copy_mac_num*4 + 4] = r_val;
			send_buf[copy_mac_num*4 + 5] = g_val;
			send_buf[copy_mac_num*4 + 6] = b_val;
			send_buf[copy_mac_num*4 + 7] = ENDFLAG;
		}
		msg_write_to_list((char *)&send_buf, send_buf[1]);
		resend_cmd_flag = 1;
	}
	recv_cmd_time = getcurrenttime();
	resend_num = 0;
/*
	//	tcp消息响应
	msgpack_sbuffer_clear(g_pk_buffer);
	msgpack_pack_array(g_pk, 7);
	msgpack_pack_uint32(g_pk, LCP_SET_RGB_RSP);
	msgpack_pack_str(g_pk, 12); 
	msgpack_pack_str_body(g_pk, mac_temp, 12);
	msgpack_pack_uint32(g_pk, erro);
	msgpack_pack_uint32(g_pk, Brightness);
	msgpack_pack_uint32(g_pk, r_val);
	msgpack_pack_uint32(g_pk, g_val);
	msgpack_pack_uint32(g_pk, b_val);

	#ifdef TCP_SEND_DEBUG
	printf("send msg_LcpSetRGB:\n");
	printhex((void *)g_pk_buffer->data, g_pk_buffer->size);
	printf("\n");
	#endif
	tcp_send(sockfd_server,(void *)g_pk_buffer->data, g_pk_buffer->size);*/
	return;
}

void msg_LcpSetMotorParams(msgpack_object* result)
{
	msgpack_object* LampId_obj  = result->via.array.ptr + 1;
	msgpack_object* Action_obj  = result->via.array.ptr + 2;
	msgpack_object* Angle_obj   = result->via.array.ptr + 3;
	msgpack_object* MotorId_obj = result->via.array.ptr + 4;
	

	char mac_temp[20] = {0};
	int str_len, Action, Angle, MotorId;
	int erro = 0;
	str_len = LampId_obj->via.str.size;
	
	if(str_len == MAC_LEN)memcpy(mac_temp, LampId_obj->via.str.ptr, MAC_LEN);
	else erro = 1;
	Action = Action_obj->via.i64;
	Angle  = Angle_obj->via.i64;
	MotorId = MotorId_obj->via.i64;

	//接受消息处理，并加入串口发送队列 ---单调电机
	unsigned char send_buf[20];
	send_buf[0] = HEADFLAG;
	send_buf[1] = 0x0C;
	send_buf[2] = MSG_SET_MOTOR;//消息ID
	send_buf[3] = 0x01;//调节灯的个数
	get_Mac_Hex(&mac_temp[MAC_offset], (unsigned char *)&send_buf[4], MAC_LEN - MAC_offset);
    /*Action, 0:stop, 1:up, 2:down, 3:left, 4:right
      Angle   in 0 ~ 2000 , the middle is 1000.
      MotorId start from 1, 2, 3...
    */
	if(Action == 0)//Action的值从msgpack协议中获得
	{
		if(MotorId == 1);
		else if(MotorId == 2);
		else if(MotorId == 3);
		else erro = 3;
		send_buf[8] = MotorId;//电机ID
		if(Angle > 2000 || Angle < 0)erro = 4;
		send_buf[9] = Angle >> 8;
		send_buf[10] = Angle & 0xFF;
	}
	else if(Action == 1)
	{
		send_buf[8] = 4;
	}
	else if(Action == 2)
	{
		send_buf[8] = 5;
	}
	else if(Action == 3)
	{
		send_buf[8] = 6;
	}
	else if(Action == 4)
	{
		send_buf[8] = 7;
	}
	else erro = 2;				
	send_buf[11] = ENDFLAG;
	if(erro == 0)
	{
		msg_write_to_list((char *)&send_buf, send_buf[1]);
	}

	//	tcp消息响应
	msgpack_sbuffer_clear(g_pk_buffer);
	msgpack_pack_array(g_pk, 2);
	msgpack_pack_uint32(g_pk, LCP_SET_MOTOR_PARAMS_RSP);
	msgpack_pack_uint32(g_pk, erro);//ResponseCode

	tcp_send(sockfd_server, (void *)g_pk_buffer->data, g_pk_buffer->size);
	return;
}

void LcpSetLampInfoOneByOne(msgpack_object* result)
{

	msgpack_object* Lamps_Id_obj = result->via.array.ptr + 1;
	
	//接受消息
	char mac_temp[20] = {0};
	int str_len, Brightness, ColorTemp, r_val, g_val, b_val;
	int erro = 1, lamp_num = 0, i, copy_mac_num = 0;
	lamp_num = Lamps_Id_obj->via.array.size;//数组成员个数
	if(lamp_num > 100)lamp_num = 100;
	msgpack_object* LampId_obj = NULL;
	msgpack_object* LampMac_obj = NULL;
	msgpack_object* LampBrightness_obj = NULL;
	msgpack_object* LampColorTemp_obj = NULL;
	msgpack_object* Lamp_r_val_obj = NULL;
	msgpack_object* Lamp_g_val_obj = NULL;
	msgpack_object* Lamp_b_val_obj = NULL;
	
	unsigned char send_buf[180] = {0};
	unsigned char msg_buf[8] = {0}, ret;

	send_buf[0] = HEADFLAG;
	send_buf[2] = MSG_SET_LAMPS;//消息ID
	for(i = 0; i < lamp_num; i++)
	{
		LampId_obj  = Lamps_Id_obj->via.array.ptr + i;
		LampMac_obj = LampId_obj->via.array.ptr;
		str_len = LampMac_obj->via.str.size;
		
		if(str_len == MAC_LEN)
		{
			LampBrightness_obj = LampId_obj->via.array.ptr + 1;
			LampColorTemp_obj  = LampId_obj->via.array.ptr + 2;
			Lamp_r_val_obj = LampId_obj->via.array.ptr + 4;
			Lamp_g_val_obj = LampId_obj->via.array.ptr + 5;
			Lamp_b_val_obj = LampId_obj->via.array.ptr + 6;
			
			Brightness = LampBrightness_obj->via.i64;
			ColorTemp  = LampColorTemp_obj->via.i64;
			r_val = Lamp_r_val_obj->via.i64;
			g_val = Lamp_g_val_obj->via.i64;
			b_val = Lamp_b_val_obj->via.i64;
			
			memcpy(mac_temp, LampMac_obj->via.str.ptr, MAC_LEN);
			get_Mac_Hex(&mac_temp[MAC_offset], (unsigned char *)&msg_buf[0], MAC_LEN - MAC_offset);

			ret = add_zbnode_light_parm((unsigned char *)&msg_buf[0], Brightness, ColorTemp, r_val, g_val, b_val, BRIGHTNESS | COLORTEMP | R_G_B);
			#ifdef TCP_RECV_DEBUG
			printf("LcpSetLampInfoOneByOne,ret=%d, Brightness:%d, colortemp:%d, R:%d, G:%d, B:%d\n", ret, Brightness, ColorTemp, r_val, g_val, b_val);
			#endif
			if(ret == 1)//调色温
			{
				msg_buf[4] = (unsigned char)Brightness;
				msg_buf[5] = (unsigned char)ColorTemp;
				msg_buf[6] = 0;
				msg_buf[7] = 0;
			}
			else if(ret == 2)//调RGB
			{
				msg_buf[4] = (unsigned char)Brightness;
				msg_buf[5] = (unsigned char)r_val;
				msg_buf[6] = (unsigned char)g_val;
				msg_buf[7] = (unsigned char)b_val;
			}
			else if(ret == 3)//调色温和RGB
			{
				msg_buf[4] = (unsigned char)Brightness;
				msg_buf[5] = (unsigned char)r_val;
				msg_buf[6] = (unsigned char)g_val;
				msg_buf[7] = (unsigned char)b_val;
			}
            #ifdef TCP_RECV_DEBUG
			printf("LcpSetLampInfoOneByOne copy_mac_num:%d\n", copy_mac_num);
			printhex(msg_buf, 8);
            #endif
			memcpy((char *)&send_buf[4 + copy_mac_num * 8], msg_buf, 8);
			copy_mac_num++;
			if(copy_mac_num >= (MAX_LAMP_SEND_LIGHT / 2))//大于等于10个灯
			{
				send_buf[1] = copy_mac_num*8 + 5;
				send_buf[3] = copy_mac_num;
				send_buf[copy_mac_num*8 + 4] = ENDFLAG;
				copy_mac_num = 0;
				msg_write_to_list((char *)&send_buf, send_buf[1]);
				resend_cmd_flag = 1;
			}
		}
		else erro = 1;
	}	

	if(copy_mac_num != 0)//小于10个灯
	{
		send_buf[1] = copy_mac_num * 8 + 5;
		send_buf[3] = copy_mac_num;
		send_buf[copy_mac_num * 8 + 4] = ENDFLAG;
		copy_mac_num = 0;
		msg_write_to_list((char *)&send_buf, send_buf[1]);
		resend_cmd_flag = 1;
	}
	recv_cmd_time = getcurrenttime();
	resend_num = 0;
	//	tcp消息响应
	msgpack_sbuffer_clear(g_pk_buffer);
	msgpack_pack_array(g_pk, 2);
	msgpack_pack_uint32(g_pk, LCP_SET_LAMP_INFO_ONE_BY_ONE_RSP);
	msgpack_pack_uint32(g_pk, 0);

	tcp_send(sockfd_server, (void *)g_pk_buffer->data, g_pk_buffer->size);
	return;
	
}

void LcpSyncDefaultSceneInfo (msgpack_object* result)//服务器同步默认情景消息函数,灯具登录时调用
{
    printf("*******LcpSyncDefaultSceneInfo!!!*******\n");
	pthread_mutex_lock(&scene_file_mutex);	
	FILE * fp = fopen(FILE_SCENE, "rb+");//可读可写方式打开一个二进制文件
	if(fp == NULL)
	{
		printf("file /mnt/scene.info is open error! \n");
	}
	else
	{
		msgpack_object* LampId_obj = result->via.array.ptr + 1;
		msgpack_object* Scene_obj  = result->via.array.ptr + 2;
		msgpack_object* Scene_One_obj  = NULL;
		msgpack_object* Scene_ID_obj   = NULL;
		msgpack_object* Brightness_obj = NULL;
		msgpack_object* ColorTemp_obj  = NULL;
		msgpack_object* R_val_obj = NULL;
		msgpack_object* G_val_obj = NULL;
		msgpack_object* B_val_obj = NULL;
		
		char mac_temp[20], mac[4], read_temp[27], scene_head_parm[27];
		int scene_num, str_len;
		str_len = LampId_obj->via.str.size;
		scene_num = Scene_obj->via.array.size;
		if(str_len == MAC_LEN)memcpy(mac_temp, LampId_obj->via.str.ptr, MAC_LEN);
		get_Mac_Hex(&mac_temp[MAC_offset], mac, MAC_LEN - MAC_offset);
		fread(scene_head_parm, 1, 27, fp);
		int i, j, ret = 1, scene_id = 0;
		int is_same = 0;
		for(i = 0; i < scene_head_parm[1]; i++)//灯具个数
		{
			fread(read_temp, 1, 27, fp);
			if(memcmp((char *)&read_temp[2], mac, 4) == 0)//比较灯的MAC地址
			{
				ret = 0;
				printf("LcpSyncDefaultSceneInfo,read num:%d\n", i);
				for(j = 0; j < scene_num; j++)//情景数为4
				{
					Scene_One_obj = Scene_obj->via.array.ptr + j;
					Scene_ID_obj  = Scene_One_obj->via.array.ptr;
					Brightness_obj= Scene_One_obj->via.array.ptr + 1;
					ColorTemp_obj = Scene_One_obj->via.array.ptr + 2;
					R_val_obj     = Scene_One_obj->via.array.ptr + 3;
					G_val_obj     = Scene_One_obj->via.array.ptr + 4;
					B_val_obj     = Scene_One_obj->via.array.ptr + 5;
					

					scene_id = Scene_ID_obj->via.u64;
                    printf("LcpSyncDefaultSceneInfo, scene_id[%d]=%d\n", i, scene_id);
					if(read_temp[(scene_id - 1)*5 + 6]  != (char)Brightness_obj->via.i64) read_temp[(scene_id - 1)*5 + 6] = (char)Brightness_obj->via.i64, is_same = 1;
					if(read_temp[(scene_id - 1)*5 + 7]  != (char)ColorTemp_obj->via.i64) read_temp[(scene_id - 1)*5 + 7] = (char)ColorTemp_obj->via.i64, is_same = 1;
					if(read_temp[(scene_id - 1)*5 + 8]  != (char)R_val_obj->via.i64) read_temp[(scene_id - 1)*5 + 8] = (char)R_val_obj->via.i64, is_same = 1;
					if(read_temp[(scene_id - 1)*5 + 9]  != (char)G_val_obj->via.i64) read_temp[(scene_id - 1)*5 + 9] = (char)G_val_obj->via.i64, is_same = 1;
					if(read_temp[(scene_id - 1)*5 + 10] != (char)B_val_obj->via.i64) read_temp[(scene_id - 1)*5 + 10] = (char)B_val_obj->via.i64, is_same = 1;	
				}
				if(read_temp[0] == Start)is_same = 1;	
				break;
			}
		}
		if(ret == 1)//文件中不存在该灯具的信息
		{
			read_temp[0]  = Start;//同步未完成
			read_temp[1]  = 1;//总重发次数为1
			read_temp[26] = 1;//当前重发次数为1
			int a, b, c, d, e;
			memcpy((char *)&read_temp[2], mac, 4);
			for(j = 0; j < scene_num; j++)
			{
				Scene_One_obj  = Scene_obj->via.array.ptr + j;
				Scene_ID_obj   = Scene_One_obj->via.array.ptr;
				Brightness_obj = Scene_One_obj->via.array.ptr + 1;
				ColorTemp_obj  = Scene_One_obj->via.array.ptr + 2;
				R_val_obj = Scene_One_obj->via.array.ptr + 3;
				G_val_obj = Scene_One_obj->via.array.ptr + 4;
				B_val_obj = Scene_One_obj->via.array.ptr + 5;
				scene_id  = Scene_ID_obj->via.u64;
                
				a = Brightness_obj->via.i64; b = ColorTemp_obj->via.i64; c = R_val_obj->via.i64; d = G_val_obj->via.i64; e = B_val_obj->via.i64;
				printf("new lamp LcpSyncDefaultSceneInfo, scene:%d, brightness:%d, coloortemp:%d, r:%d, g:%d, b:%d \n", scene_id, a, b, c, d, e);
				read_temp[(scene_id - 1)*5 + 6]  = (char)Brightness_obj->via.i64;
				read_temp[(scene_id - 1)*5 + 7]  = (char)ColorTemp_obj->via.i64;
				read_temp[(scene_id - 1)*5 + 8]  = (char)R_val_obj->via.i64;
				read_temp[(scene_id - 1)*5 + 9]  = (char)G_val_obj->via.i64;
				read_temp[(scene_id - 1)*5 + 10] = (char)B_val_obj->via.i64;	
			}
		}
		if(is_same == 1 || ret == 1)//如果灯具情景模式更改或者新增，发送同步命令并且更改文件内容
		{
			struct scene_rsp temp;
			temp.msg_head = 0x7E;
			temp.msg_len = sizeof(struct scene_rsp);
			temp.msg_id = MSG_Scene_Sync;
			memcpy(temp.lamp_id, (char *)&read_temp[2], 4);
			temp.lamp_value_1[0] = read_temp[6];  temp.lamp_value_1[1] = read_temp[7];  temp.lamp_value_1[2] = read_temp[6];
			temp.lamp_value_1[3] = read_temp[8];  temp.lamp_value_1[4] = read_temp[9];  temp.lamp_value_1[5] = read_temp[10];
			temp.lamp_value_2[0] = read_temp[11]; temp.lamp_value_2[1] = read_temp[12]; temp.lamp_value_2[2] = read_temp[11];
			temp.lamp_value_2[3] = read_temp[13]; temp.lamp_value_2[4] = read_temp[14]; temp.lamp_value_2[5] = read_temp[15];
			temp.lamp_value_3[0] = read_temp[16]; temp.lamp_value_3[1] = read_temp[17]; temp.lamp_value_3[2] = read_temp[16];
			temp.lamp_value_3[3] = read_temp[18]; temp.lamp_value_3[4] = read_temp[19]; temp.lamp_value_3[5] = read_temp[20];
			temp.lamp_value_4[0] = read_temp[21]; temp.lamp_value_4[1] = read_temp[22]; temp.lamp_value_4[2] = read_temp[21];
			temp.lamp_value_4[3] = read_temp[23]; temp.lamp_value_4[4] = read_temp[24]; temp.lamp_value_4[5] = read_temp[25];
			temp.msg_end = 0x5E;

			msg_write_to_list((char *)&temp, sizeof(struct scene_rsp));
			read_temp[0] = Start;
			read_temp[1] = 1;//总重发次数为1
			read_temp[26] = 1;//当前重发次数为1
			if(is_same == 1)fseek(fp, -27, SEEK_CUR);
			else if(ret == 1)fseek(fp, 0, SEEK_CUR);//如果是新的灯具,就重新增加27个字节存储该灯具信息
			fwrite(read_temp, 1, 27, fp);
			fseek(fp, 0, SEEK_SET);//定位到文件开头
			scene_head_parm[0] = Start;//标记未同步完成
			if(ret == 1)scene_head_parm[1]++;//但前存储的灯具个数加1
			fwrite(scene_head_parm, 1, 27, fp);//更改文件开头的27个字节信息

			scene_info_flag = 1;
			send_scene_info_count = 0;
		}
		fclose(fp);
		pthread_mutex_unlock(&scene_file_mutex);

		msgpack_sbuffer_clear(g_pk_buffer);
		msgpack_pack_array(g_pk, 3);
		msgpack_pack_uint32(g_pk, LCP_SYNC_DEFAULT_SCENE_INFO_RSP);
		msgpack_pack_str(g_pk, MAC_LEN); 
		msgpack_pack_str_body(g_pk, mac_temp, MAC_LEN);
		msgpack_pack_uint32(g_pk, 0);

		tcp_send(sockfd_server, (void *)g_pk_buffer->data, g_pk_buffer->size);
	}
}

void LcpSyncLampListByCtrlPanel(msgpack_object* result)//只有在服务器上添加过的控制器登录时调用
{
	FILE * fp = fopen(FILE_Ctrl, "rb+");
	if(fp == NULL)
	{
		printf("file /mnt/ctrl_info is open error! \n");
		return;
	}
	else
	{
		msgpack_object* PanelId_obj = result->via.array.ptr + 1;
		msgpack_object* LampIds_obj = result->via.array.ptr + 2;
		msgpack_object* LampId_obj  = NULL;
	
		char mac_temp[20], mac[4], read_temp[612], ctrl_head_parm[12], pad_mac[20];
		int lamp_num, is_write = 0;
		lamp_num = LampIds_obj->via.array.size;
        memset(pad_mac, 0, sizeof(pad_mac));
		memcpy(pad_mac, PanelId_obj->via.str.ptr, MAC_LEN);//获取面板的MAC地址
		get_Mac_Hex(&pad_mac[MAC_offset], mac, MAC_LEN - MAC_offset);
		printf("LcpSyncLampListByCtrlPanel, bind lamps num:%d\n", lamp_num);
		fread(ctrl_head_parm, 1, 12, fp);
		int i, j, k, ret = 1, scene_id;
		int is_same = 0;
		for(i = 0; i < ctrl_head_parm[0]; i++)//在文件中查找控制器 MAC,找到则跳出
		{
			fread(read_temp, 1, 612, fp);
			if(memcmp((char *)&read_temp[0], mac, 4) == 0)
			{
				ret = 0;//在文件中找到了控制器
				printf("read num:%d\n", i);	 
				break;
			}
		}

		if(ret == 1)//在文件中文未找到记录,新增控制器 MAC
		{
			memset(read_temp, 0, 612);
            #ifdef DEBUG
            printf("new pad,LcpSyncLampListByCtrlPanel, the pad mac:\n");
			printhex((uint8 *)&mac, 4);	
            #endif
			memcpy(read_temp, mac, 4);
			read_temp[4] = Start;
			read_temp[5] = (unsigned char)lamp_num;//面板下灯具个数
			is_write = 1;//标志增加控制器成功
		}
		int check_flag = 0;//检查标志位
		for(j = 0; j < lamp_num; j++)
		{
			LampId_obj = LampIds_obj->via.array.ptr + j;
            memset(mac_temp, 0, sizeof(mac_temp));
			memcpy(mac_temp, LampId_obj->via.str.ptr, MAC_LEN);//获取灯的MAC地址
			get_Mac_Hex(&mac_temp[MAC_offset], mac, MAC_LEN - MAC_offset);
			if(ret == 1)//新增
			{
				read_temp[12 + 6*j] = Start;//该灯具等待绑定
				read_temp[12 + 6*j + 1] = 0;
				memcpy((char *)&read_temp[12 + 6*j + 2], mac, 4);//将灯的MAC地址存入文件中
			}
			else //文件中存在控制器
			{
				for(k = 0; k < read_temp[5]; k++)//控制器下的灯具个数
				{
					if(memcmp((char *)&read_temp[12 + 6*k + 2], mac, 4) == 0)//查找该控制器的灯具绑定情况
					{
						check_flag = 1;
						read_temp[12 + 6*k + 1] = 1;//该控制器中存在该灯就将这位置为1, 并且判断下一个
						break;
					}
				}
				if(0 == check_flag)//说明控制器下没有该灯具
				{
					memcpy((char *)&read_temp[12 + 6*k + 2], mac, 4);//在最后一个灯的后面开始添加灯具信息
					read_temp[12 + 6*k + 1] = 1;
					read_temp[12 + 6*k] = Start;
					read_temp[4] = Start;//标记该控制器下的灯具未全部绑定
					read_temp[5]++;//灯具个数加1
					is_write = 1;
				}
			}
			check_flag = 0;
		}

		if(ret == 0)//说明在文件中匹配到了控制器
		{
			for(k = 0; k < read_temp[5]; k++)
			{
                printf("LcpSyncLampListByCtrlPanel,read_temp[%d]=%d\n",12+6*k+1, read_temp[12 + 6*k + 1]);
				if(read_temp[12 + 6*k + 1] == 0)
				{
					read_temp[12 + 6*k] = Delete;
					read_temp[4] = Start;
					is_write = 1;
				}
				else
				{
					read_temp[12 + 6*k + 1] = 0;
				}
			}
			fseek(fp, -612, SEEK_CUR);
		}
		else fseek(fp, 0, SEEK_END);
		
		if(1 == is_write)
		{
			fwrite(read_temp, 1, 612, fp);
			printf("is_write,LcpSyncLampListByCtrlPanel, pad bind lamps num:%d\n", read_temp[5]);
		
			if(ret == 1)ctrl_head_parm[0]++;
			ctrl_head_parm[1] = Start;//设置没有绑定
			fseek(fp, 0, SEEK_SET);
			fwrite(ctrl_head_parm, 1, 12, fp);//将文件信息写入FILE_Ctrl中
			
			check_control_flag_minor = 1;
			check_control_flag_main = 1;
		}
		fclose(fp);

		msgpack_sbuffer_clear(g_pk_buffer);
		msgpack_pack_array(g_pk, 3);
		msgpack_pack_uint32(g_pk, LCP_SYNC_LAMP_LIST_BY_CTRL_PANEL_RSP);//PanelId
		msgpack_pack_str(g_pk, MAC_LEN); 
		msgpack_pack_str_body(g_pk, PanelId_obj->via.str.ptr, MAC_LEN);
		msgpack_pack_uint32(g_pk, 0);

		tcp_send(sockfd_server, (void *)g_pk_buffer->data, g_pk_buffer->size);
	}
}

int tcp_recive_cmd(msgpack_unpacked* result)
{
 	msgpack_object* msg_id_obj = NULL;
    uint32_t msg_id;
    
    if (MSGPACK_OBJECT_ARRAY != result->data.type)
    {
        printf("tcp_recive_cmd ...1\n");
		return 1;
    }

    msg_id_obj = result->data.via.array.ptr;
    if (MSGPACK_OBJECT_POSITIVE_INTEGER != msg_id_obj->type)
    {
        printf("tcp_recive_cmd...2\n");
		return 2;
    }
	msg_id = msg_id_obj->via.u64;
	printf("*****************************************************************msg_id:%d\n", msg_id);
    switch (msg_id)
    {
		case LCP_LINK_INIT_RSP: msg_LcpLinkInitRsp(&(result->data));break;
		case LCP_LAMP_LOGIN_RSP: msg_LcpLampLoginRsp(&(result->data));break;
		case LCP_LAMP_LOGOUT_RSP: printf("*****LCP_LAMP_LOGOUT_RSP*****\n");break;
		case LCP_LINK_KEEPALIVE_RSP: msg_LCP_LINK_KEEPALIVE_RSP(&(result->data));break;
		case LCP_SET_BRIGHTNESS_COLOR_TEMP: msg_LcpSetBrightnessColorTemp(&(result->data));break;
		case LCP_SET_RGB: msg_LcpSetRGB(&(result->data));break;
		case LCP_GET_LAMP_INFO: msg_LcpGetLampInfo(&(result->data));break;
		case LCP_SET_GROUP_LAMP_INFO: msg_LcpSetGroupLampInfo(&(result->data));break;
		case LCP_SET_MOTOR_PARAMS: msg_LcpSetMotorParams(&(result->data));break;
		case LCP_SET_LAMP_INFO_ONE_BY_ONE: LcpSetLampInfoOneByOne(&(result->data));break;
		case LCP_CTRL_PANEL_LOGIN_RSP: msg_LcpPadLoginRsp(&(result->data));break;
		case LCP_SYNC_DEFAULT_SCENE_INFO: LcpSyncDefaultSceneInfo(&(result->data));break;
		case LCP_SYNC_LAMP_LIST_BY_CTRL_PANEL: LcpSyncLampListByCtrlPanel(&(result->data));break;
		default: printf("tcp_recive_cmd switch default\n");break;
    }
	return 0;
}



int tcp_recive_unpack(char *msg,int len)
{
	msgpack_unpacker_reserve_buffer(g_unpk, len);
	memcpy(msgpack_unpacker_buffer(g_unpk), msg, len);
	msgpack_unpacker_buffer_consumed(g_unpk, len);

	msgpack_unpacked result;
	msgpack_unpacked_init(&result);
	int ret;
	while (1) 
	{
    	ret = msgpack_unpacker_next(g_unpk, &result);
		if(ret == MSGPACK_UNPACK_SUCCESS)
		{
			tcp_recive_cmd(&result);
		}
		else if(ret == MSGPACK_UNPACK_CONTINUE)
		{
			break;
		}
		else
		{
			msgpack_unpacker_free(g_unpk);
			g_unpk = msgpack_unpacker_new(BUF_MSGPACK);
			break;
		}
	}
	return 1;
}

void get_mac_str(char *source,char *target)
{
	int i = 0;
	for(i = 0; i < 6; i++)sprintf(&target[2 * i], "%02X", (unsigned int)&source[2 + i]);
	return;
}

uint8  get_Mac_Hex(char *mac_source,uint8 *mac_target,uint8 len)
{
	uint8 i = 0, j = 0;
	uint8 temp1 = 0, temp2 = 0;
	for(i = 0; i < len; i++)
	{
		if((mac_source[i] >= '0') && (mac_source[i] <= '9'))temp1 = mac_source[i] - '0';
		else if((mac_source[i] >= 'A') && (mac_source[i] <= 'F'))temp1 = mac_source[i] - 'A' + 10;
		else return 1;
		i++;
		if((mac_source[i] >= '0') && (mac_source[i] <= '9'))temp2 = mac_source[i] - '0';
		else if((mac_source[i] >= 'A') && (mac_source[i] <= 'F'))temp2 = mac_source[i] - 'A' + 10;
		else return 1;
		mac_target[j] = 16 * temp1 + temp2;
		j++;
		i++;
	}
	if(i == len)return 0;
	return 1;
}



void send_lamp_loginout(unsigned char *temp_node_mac,uint8 type)
{
	char mac_tempbuff[20] = {0};
	unsigned m;
	for(m = 0; m < 6; m++)sprintf(&mac_tempbuff[3 * m], "%02X:", temp_node_mac[2 + m]);//将mac地址存入mac_tempbuff中
	if(type < 101)//灯具
	{
		msgpack_sbuffer_clear(g_pk_buffer);
		msgpack_pack_array(g_pk, 2);
		msgpack_pack_uint32(g_pk, LCP_LAMP_LOGOUT);
		msgpack_pack_str(g_pk, MAC_LEN); 
		msgpack_pack_str_body(g_pk, mac_tempbuff, MAC_LEN);
		tcp_send(sockfd_server, (void *)g_pk_buffer->data, g_pk_buffer->size);
	}
	else//控制器
	{
		msgpack_sbuffer_clear(g_pk_buffer);
		msgpack_pack_array(g_pk, 2);
		msgpack_pack_uint32(g_pk, LCP_CTRL_PANEL_LOGOUT);
		msgpack_pack_array(g_pk, 1);
		msgpack_pack_array(g_pk, 2);
		msgpack_pack_str(g_pk, MAC_LEN); 
		msgpack_pack_str_body(g_pk, mac_tempbuff, MAC_LEN);
		msgpack_pack_uint32(g_pk, type);
		tcp_send(sockfd_server, (void *)g_pk_buffer->data, g_pk_buffer->size);
	}
	return;
}

