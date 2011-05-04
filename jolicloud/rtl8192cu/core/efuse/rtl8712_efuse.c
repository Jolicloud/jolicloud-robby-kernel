/******************************************************************************
 *
 * Copyright(c) 2007 - 2010 Realtek Corporation. All rights reserved.
 *                                        
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110, USA
 *
 *
 ******************************************************************************/
#define _RTL8712_EFUSE_C_

#include <drv_conf.h>
#include <osdep_service.h>
#include <drv_types.h>

#include <rtl8712_efuse.h>
//------------------------------------------------------------------------------
static void efuse_reg_ctrl(_adapter *padapter, u8 bPowerOn)
{
	u16 	tmpV16 	= 0;			
	
	if(_TRUE ==bPowerOn){

		// 1.2V Power: From VDDON with Power Cut(0x0000h[15]), defualt valid
		tmpV16 = rtw_read16(padapter,REG_SYS_ISO_CTRL);
		if( ! (tmpV16 & PWC_EV12V ) ){
			tmpV16 |= PWC_EV12V ;
			 rtw_write16(padapter,REG_SYS_ISO_CTRL,tmpV16);
		}
		// Reset: 0x0000h[28], default valid
		tmpV16 = rtw_read16(padapter,REG_SYS_FUNC_EN);
		if( !(tmpV16 & FEN_ELDR) ){
			tmpV16 |= FEN_ELDR ;
			rtw_write16(padapter,REG_SYS_FUNC_EN,tmpV16);
		}
		
		// Clock: Gated(0x0008h[5]) 8M(0x0008h[1]) clock from ANA, default valid
		tmpV16 = rtw_read16(padapter,REG_SYS_CLKR);
		if( (!(tmpV16 & LOADER_CLK_EN) )  ||(!(tmpV16 & ANA8M) ) ){
			tmpV16 |= (LOADER_CLK_EN |ANA8M ) ;
			rtw_write16(padapter,REG_SYS_CLKR,tmpV16);
		}		
	}
}

/*
 * Before write E-Fuse, this function must be called.
 */
u8 rtw_efuse_reg_init(_adapter *padapter)
{
	efuse_reg_ctrl(padapter, _TRUE);
	return _TRUE;
}

void rtw_efuse_reg_uninit(_adapter *padapter)
{
	efuse_reg_ctrl(padapter, _FALSE);
}

//------------------------------------------------------------------------------
u8 efuse_one_byte_read(_adapter *padapter, u16 addr, u8 *data)
{
	u8 tmpidx = 0, bResult;

	// -----------------e-fuse reg ctrl ---------------------------------				
	rtw_write8(padapter, EFUSE_CTRL+1, (u8)(addr&0xFF)); //address
	rtw_write8(padapter, EFUSE_CTRL+2, ((u8)((addr>>8)&0x03)) | (rtw_read8(padapter, EFUSE_CTRL+2)&0xFC));		

	// use default program time 5000 ns
	rtw_write32(padapter,   REG_EFUSE_CTRL, rtw_read32(padapter,  REG_EFUSE_CTRL)&( ~EF_FLAG));//read cmd

	// wait for complete
	while (!(0x80 & rtw_read8(padapter, EFUSE_CTRL+3)) && (tmpidx < 100))
		tmpidx++;

	if (tmpidx < 100) {
		*data = rtw_read8(padapter, EFUSE_CTRL);
		RT_TRACE(_module_rtl8712_efuse_c_,_drv_info_,("====Read Efuse addr=0x%x value=0x%x=====\n", addr, *data));
		bResult = _TRUE;
	} else {
		*data = 0xff;
		RT_TRACE(_module_rtl8712_efuse_c_,_drv_err_,("====Read Efuse fail!!! addr=0x%x=====\n", addr));
		bResult = _FALSE;
	}
	return bResult;
}
//------------------------------------------------------------------------------
static u8 efuse_one_byte_write(_adapter *padapter, u16 addr, u8 data)
{
	u8 tmpidx = 0, bResult;

	// -----------------e-fuse reg ctrl ---------------------------------
	rtw_write8(padapter, EFUSE_CTRL+1, (u8)(addr&0xFF)); //address
	rtw_write8(padapter, EFUSE_CTRL+2, ((u8)((addr>>8)&0x03)) | (rtw_read8(padapter, EFUSE_CTRL+2)&0xFC));
	rtw_write8(padapter, EFUSE_CTRL, data); //data
	
	// use default program time 5000 ns
	rtw_write32(padapter,REG_EFUSE_CTRL, rtw_read32(padapter,REG_EFUSE_CTRL)|EF_FLAG);//write cmd	

	// wait for complete
	while ((0x80 &  rtw_read8(padapter, EFUSE_CTRL+3)) && (tmpidx < 100))
		tmpidx++;

	if (tmpidx < 100) {
		RT_TRACE(_module_rtl8712_efuse_c_,_drv_info_,("====Write Efuse addr=0x%x value=0x%x=====\n", addr, data));
		bResult = _TRUE;
	} else {
		RT_TRACE(_module_rtl8712_efuse_c_,_drv_err_,("====Write Efuse fail!! addr=0x%x value=0x%x=====\n", addr, data));
		bResult = _FALSE;
	}

	return bResult;	
}
//------------------------------------------------------------------------------
static u8 efuse_one_byte_rw(_adapter *padapter, u8 bRead, u16 addr, u8 *data)
{
	u8 tmpidx = 0, tmpv8 = 0, bResult;
	
	// -----------------e-fuse reg ctrl ---------------------------------				
	rtw_write8(padapter, EFUSE_CTRL+1, (u8)(addr&0xFF)); //address
	tmpv8 = ((u8)((addr>>8)&0x03)) | (rtw_read8(padapter, EFUSE_CTRL+2)&0xFC);
	rtw_write8(padapter, EFUSE_CTRL+2, tmpv8);

	if (_TRUE == bRead) {
		rtw_write8(padapter, EFUSE_CTRL+3,  0x72);//read cmd		

		while (!(0x80 & rtw_read8(padapter, EFUSE_CTRL+3)) && (tmpidx < 100))
			tmpidx++;

		if (tmpidx < 100) {		
			*data = rtw_read8(padapter, EFUSE_CTRL);
			bResult = _TRUE;
		} else {
			*data = 0;
			RT_TRACE(_module_rtl8712_efuse_c_,_drv_err_,("====Read Efuse Fail!! addr=0x%x =====\n", addr));		
			bResult = _FALSE;
		}
	} else {
		rtw_write8(padapter, EFUSE_CTRL, *data);//data
		rtw_write8(padapter, EFUSE_CTRL+3, 0xF2);//write cmd

		while ((0x80 & rtw_read8(padapter, EFUSE_CTRL+3)) && (tmpidx < 100))
			tmpidx++;

		if (tmpidx < 100) {
			RT_TRACE(_module_rtl8712_efuse_c_,_drv_info_,("====Write Efuse addr=0x%x value=0x%x =====\n", addr, *data));
			bResult = _TRUE;
		} else {
			RT_TRACE(_module_rtl8712_efuse_c_,_drv_err_,("====Write Efuse Fail!! addr =0x%x value=0x%x =====\n", addr, *data));
			bResult = _FALSE;
		}
	}

	return bResult;	
}
//------------------------------------------------------------------------------
static u8 efuse_is_empty(_adapter *padapter, u8 *empty)
{
	u8 value, ret = _TRUE;

	// read one byte to check if E-Fuse is empty
	if (efuse_one_byte_rw(padapter, _TRUE, 0, &value) == _TRUE) {
		if (0xFF == value) *empty = _TRUE;
		else *empty = _FALSE;
	} else {
		// read fail
		RT_TRACE(_module_rtl871x_mp_ioctl_c_,_drv_emerg_,
			("efuse_is_empty: fail!!=====\n"));
		ret = _FALSE;
	}

	return ret;
}
//------------------------------------------------------------------------------
int rtw_efuse_get_max_phy_size(_adapter *padapter)
{
	u16 pre_pg_data_saddr = 507;
	
	u16 pre_pg_data_size = 5;
	u8 pre_pg_data[5];
	u16 i;
	padapter->eeprompriv.efuse_phy_max_size = 509 ;	//reserved 3 byte for safe access
	
	for(i=0;i <pre_pg_data_size;i++){
		efuse_one_byte_read(padapter,pre_pg_data_saddr+i,&pre_pg_data[i]);
	}
	if(	(pre_pg_data[0]==0x03) && (pre_pg_data[1]==0x00) && (pre_pg_data[2]==0x00) && 
		(pre_pg_data[3]==0x00) && (pre_pg_data[4]==0x0C) ){
		padapter->eeprompriv.efuse_phy_max_size = 504;//reserved 3 byte for safe access
	}
	printk("%s...eeprompriv.efuse_phy_max_size(%d)\n",__FUNCTION__,padapter->eeprompriv.efuse_phy_max_size);
	return padapter->eeprompriv.efuse_phy_max_size;
}
//------------------------------------------------------------------------------
static u8 calculate_word_cnts(const u8 word_en)
{
	u8 word_cnts = 0;
	u8 word_idx;

	for (word_idx = 0; word_idx < EFUSE_PGPKG_MAX_WORDS; word_idx++) {
		if (!(word_en & BIT(word_idx))) word_cnts++; // 0 : write enable		
	}
	return word_cnts;
}
//------------------------------------------------------------------------------
static void pgpacket_copy_data(const u8 word_en, const u8 *sourdata, u8 *targetdata)
{
	u8 tmpindex = 0;
	u8 word_idx, byte_idx;

	for (word_idx = 0; word_idx < EFUSE_PGPKG_MAX_WORDS; word_idx++) {
		if (!(word_en&BIT(word_idx))) {	
			byte_idx = word_idx * 2;
			targetdata[byte_idx] = sourdata[tmpindex++];
			targetdata[byte_idx + 1] = sourdata[tmpindex++];
		}
	}
}
//------------------------------------------------------------------------------
u16 rtw_efuse_get_current_phy_size(_adapter *padapter)
{
	int bContinual = _TRUE;

	u16 efuse_addr = 0;
	u8 hoffset = 0, hworden = 0;
	u8 efuse_data, word_cnts = 0;

	while (bContinual && efuse_one_byte_read(padapter, efuse_addr, &efuse_data) &&
	       (efuse_addr < padapter->eeprompriv.efuse_phy_max_size))
	{
		if (efuse_data != 0xFF) {
			hoffset = (efuse_data >> 4) & 0x0F;
			hworden =  efuse_data & 0x0F;							
			word_cnts = calculate_word_cnts(hworden);
			//read next header
			efuse_addr = efuse_addr + (word_cnts * 2) + 1;
		} else {
			bContinual = _FALSE ;			
		}
	}

	return efuse_addr;
}
//------------------------------------------------------------------------------
u8 rtw_efuse_pg_packet_read(_adapter *padapter, u8 offset, u8 *data)
{
	u8 ReadState = EFUSE_PG_STATE_HEADER;	
	
	u8 bContinual = _TRUE;	

	u8 efuse_data,word_cnts=0;
	u16 efuse_addr = 0;
	u8 hoffset=0,hworden=0;	
	u8 tmpidx=0;
	u8 tmpdata[8];
	
	if(data==NULL)	return _FALSE;
	if(offset>0x0f)	return _FALSE;	
	
	_rtw_memset(data,0xFF,sizeof(u8)* EFUSE_PGPKT_DATA_SIZE);	
	_rtw_memset(tmpdata,0xFF,sizeof(u8)* EFUSE_PGPKT_DATA_SIZE );		
	
	while(bContinual && (efuse_addr  < padapter->eeprompriv.efuse_phy_max_size) ){			
		//-------  Header Read -------------
		if(ReadState & EFUSE_PG_STATE_HEADER){
			if(efuse_one_byte_read(padapter,efuse_addr ,&efuse_data)&&(efuse_data!=0xFF)){				
				hoffset = (efuse_data>>4) & 0x0F;
				hworden =  efuse_data & 0x0F;									
				word_cnts = calculate_word_cnts(hworden);
				
				if(hoffset==offset){					
					for(tmpidx = 0;tmpidx< word_cnts*2 ;tmpidx++){
						if(efuse_one_byte_read(padapter,efuse_addr+1+tmpidx ,&efuse_data) ){
							tmpdata[tmpidx] = efuse_data;						
						}					
					}
					ReadState = EFUSE_PG_STATE_DATA;	
					
				}
				else{//read next header	
					efuse_addr = efuse_addr + (word_cnts*2)+1;
					ReadState = EFUSE_PG_STATE_HEADER; 
				}
				
			}
			else{
				bContinual = _FALSE;
			}
		}		
		//-------  Data section Read -------------
		else if(ReadState & EFUSE_PG_STATE_DATA){
			pgpacket_copy_data(hworden,tmpdata,data);
			efuse_addr = efuse_addr + (word_cnts*2)+1;
			ReadState = EFUSE_PG_STATE_HEADER; 
		}		
	}			
	
	
	if(	(data[0]==0xff) &&(data[1]==0xff) && (data[2]==0xff)  && (data[3]==0xff) &&
		(data[4]==0xff) &&(data[5]==0xff) && (data[6]==0xff)  && (data[7]==0xff))
		return _FALSE;
	else
		return _TRUE;
}
//------------------------------------------------------------------------------
static u8 pgpacket_write_data(_adapter *padapter, const u16 efuse_addr, const u8 word_en, const u8 *data)
{		
	u16 start_addr = efuse_addr;

	u8 badworden = 0x0F;

	u8 word_idx, byte_idx;

	u16 tmpaddr = 0;
	u8 tmpdata[EFUSE_PGPKT_DATA_SIZE];


	_rtw_memset(tmpdata, 0xff, EFUSE_PGPKT_DATA_SIZE);

	for (word_idx = 0; word_idx < EFUSE_PGPKT_DATA_SIZE; word_idx++) {
		if (!(word_en & BIT(word_idx))) {
			tmpaddr = start_addr;
			byte_idx = word_idx * 2;
			efuse_one_byte_write(padapter, start_addr++, data[byte_idx]);
			efuse_one_byte_write(padapter, start_addr++, data[byte_idx + 1]);

			efuse_one_byte_read(padapter, tmpaddr, &tmpdata[byte_idx]);
			efuse_one_byte_read(padapter, tmpaddr + 1, &tmpdata[byte_idx+1]);
			if ((data[byte_idx] != tmpdata[byte_idx]) || (data[byte_idx+1] != tmpdata[byte_idx+1])) {
				badworden &= (~BIT(word_idx));
			}
		}
	}

	return badworden;
}

//------------------------------------------------------------------------------
u8 rtw_efuse_pg_packet_write(_adapter *padapter, const u8 offset, const u8 word_en, const u8 *data)
{
	u8 WriteState = EFUSE_PG_STATE_HEADER;		

	u8 bContinual = _TRUE,bDataEmpty=_TRUE,bResult = _TRUE;
	u16 efuse_addr = 0;
	u8 efuse_data;

	u8 pg_header = 0;
	u16 remain_size = 0,curr_size = 0;
	u16 tmp_addr=0;
	
	u8 tmp_word_cnts=0,target_word_cnts=0;
	u8 tmp_header,match_word_en,tmp_word_en;

	u8 word_idx;

	PGPKT_STRUCT target_pkt;	
	PGPKT_STRUCT tmp_pkt;
	
	u8 originaldata[sizeof(u8)*8];
	u8 tmpindex = 0,badworden = 0x0F;

	static int repeat_times = 0;

	if( (curr_size= rtw_efuse_get_current_phy_size(padapter)) >= padapter->eeprompriv.efuse_phy_max_size){
		return _FALSE;
	}
	remain_size = padapter->eeprompriv.efuse_phy_max_size - curr_size;

	
	
	target_pkt.offset = offset;
	target_pkt.word_en= word_en;
	_rtw_memset(target_pkt.data,0xFF,sizeof(u8)*8);
	pgpacket_copy_data(word_en,data,target_pkt.data);
	target_word_cnts = calculate_word_cnts(target_pkt.word_en);	
	
	if( remain_size <  (target_word_cnts*2+1)){		
		return _FALSE; //target_word_cnts + pg header(1 byte)
	}
	
	
	while( bContinual && (efuse_addr  < padapter->eeprompriv.efuse_phy_max_size) ){
		
		if(WriteState==EFUSE_PG_STATE_HEADER){	
			bDataEmpty=_TRUE;
			badworden = 0x0F;		
			//************  so *******************	
			if( efuse_one_byte_read(padapter,efuse_addr ,&efuse_data) &&(efuse_data!=0xFF)){ 	
				tmp_header  =  efuse_data;
				
				tmp_pkt.offset 	= (tmp_header>>4) & 0x0F;
				tmp_pkt.word_en 	= tmp_header & 0x0F;					
				tmp_word_cnts 	= calculate_word_cnts(tmp_pkt.word_en);

				//************  so-1 *******************
				if(tmp_pkt.offset  != target_pkt.offset){
					efuse_addr = efuse_addr + (tmp_word_cnts*2) +1; //Next pg_packet
					WriteState = EFUSE_PG_STATE_HEADER;
				}
				else{	
					//************  so-2 *******************
					for(tmpindex=0 ; tmpindex<(tmp_word_cnts*2) ; tmpindex++){
						if(efuse_one_byte_read(padapter,(efuse_addr+1+tmpindex) ,&efuse_data)&&(efuse_data != 0xFF)){
							bDataEmpty = _FALSE;	
						}
					}	
					//************  so-2-1 *******************
					if(bDataEmpty == _FALSE){						
						efuse_addr = efuse_addr + (tmp_word_cnts*2) +1; //Next pg_packet	
						WriteState=EFUSE_PG_STATE_HEADER;
					}
					else{//************  so-2-2 *******************
						match_word_en = 0x0F;					
						for(word_idx=0;word_idx<4;word_idx++){
							if(   !( (target_pkt.word_en&BIT(word_idx))|(tmp_pkt.word_en&BIT(word_idx))  )){
								 match_word_en &= (~BIT(word_idx));
							}	
						}					
						//************  so-2-2-A *******************
						if((match_word_en&0x0F)!=0x0F){							
							badworden = pgpacket_write_data(padapter,efuse_addr+1, tmp_pkt.word_en ,target_pkt.data);
							
							//************  so-2-2-A-1 *******************
							//############################
							if(0x0F != (badworden&0x0F)){														
								u8 reorg_offset = offset;
								u8 reorg_worden=badworden;								
								rtw_efuse_pg_packet_write(padapter,reorg_offset,reorg_worden,originaldata);	
							}	
							//############################						

							tmp_word_en = 0x0F;						
							for(word_idx=0;word_idx<4;word_idx++){
								if(  (target_pkt.word_en&BIT(word_idx))^(match_word_en&BIT(word_idx))  ){
									tmp_word_en &= (~BIT(word_idx));
								}
							}				
							//************  so-2-2-A-2 *******************	
							if((tmp_word_en&0x0F)!=0x0F){
								//reorganize other pg packet														
								efuse_addr = rtw_efuse_get_current_phy_size(padapter);								
								//===========================
								target_pkt.offset = offset;
								target_pkt.word_en= tmp_word_en;					
								//===========================
							}else{								
								bContinual = _FALSE;								
							}	
							WriteState=EFUSE_PG_STATE_HEADER;
							repeat_times++;
							if(repeat_times>EFUSE_REPROG_THRESHOLD){
								bContinual = _FALSE;
								bResult = _FALSE;
							}
						}
						else{//************  so-2-2-B *******************
							//reorganize other pg packet						
							efuse_addr = efuse_addr + (2*tmp_word_cnts) +1;//next pg packet addr							
							//===========================
							target_pkt.offset = offset;
							target_pkt.word_en= target_pkt.word_en;					
							//===========================	
							WriteState=EFUSE_PG_STATE_HEADER;
						}
						
					}				
					
				}				
				
			}
			else	{ //************  s1: header == oxff  *******************	
				curr_size = rtw_efuse_get_current_phy_size(padapter);
				remain_size = padapter->eeprompriv.efuse_phy_max_size - curr_size;			
				
				if( remain_size <  (target_word_cnts*2+1)) //target_word_cnts + pg header(1 byte)
				{					
					bContinual = _FALSE;
					bResult = _FALSE;	
				}
				else{
					pg_header = ((target_pkt.offset << 4)&0xf0) |target_pkt.word_en;							

					efuse_one_byte_write(padapter,efuse_addr, pg_header);
					efuse_one_byte_read(padapter,efuse_addr, &tmp_header);		
			
					if(tmp_header == pg_header){ //************  s1-1 *******************								
						WriteState = EFUSE_PG_STATE_DATA;						
					}
					else if(tmp_header == 0xFF){//************  s1-3: if Write or read func doesn't work *******************		
						//efuse_addr doesn't change
						WriteState = EFUSE_PG_STATE_HEADER;					
						repeat_times++;
						if(repeat_times>EFUSE_REPROG_THRESHOLD){
							bContinual = _FALSE;
							bResult = _FALSE;
						}
					}
					else{//************  s1-2 : fixed the header procedure *******************							
						tmp_pkt.offset = (tmp_header>>4) & 0x0F;
						tmp_pkt.word_en=  tmp_header & 0x0F;					
						tmp_word_cnts =  calculate_word_cnts(tmp_pkt.word_en);
																												
						//************  s1-2-A :cover the exist data *******************
						_rtw_memset(originaldata,0xff,sizeof(u8)*8);
						
						if(rtw_efuse_pg_packet_read( padapter, tmp_pkt.offset,originaldata)){	//check if data exist 											
							badworden = pgpacket_write_data(padapter,efuse_addr+1,tmp_pkt.word_en,originaldata);	
							//############################
							if(0x0F != (badworden&0x0F)){														
								u8 reorg_offset = tmp_pkt.offset;
								u8 reorg_worden=badworden;								
								rtw_efuse_pg_packet_write(padapter,reorg_offset,reorg_worden,originaldata);	
								efuse_addr = rtw_efuse_get_current_phy_size(padapter);							
							}
							//############################
							else{
								efuse_addr = efuse_addr + (tmp_word_cnts*2) +1; //Next pg_packet							
							}						
						}
						 //************  s1-2-B: wrong address*******************
						else{
							efuse_addr = efuse_addr + (tmp_word_cnts*2) +1; //Next pg_packet						
						}	
						WriteState=EFUSE_PG_STATE_HEADER;	
						repeat_times++;
						if(repeat_times>EFUSE_REPROG_THRESHOLD){
							bContinual = _FALSE;
							bResult = _FALSE;
						}
					}
				}

			}

		}
		//write data state
		else if(WriteState==EFUSE_PG_STATE_DATA) {//************  s1-1  *******************
			badworden = 0x0f;
			badworden = pgpacket_write_data(padapter,efuse_addr+1,target_pkt.word_en,target_pkt.data);	
			if((badworden&0x0F)==0x0F){ //************  s1-1-A *******************
				bContinual = _FALSE;
			}
			else{//reorganize other pg packet //************  s1-1-B *******************
				efuse_addr = efuse_addr + (2*target_word_cnts) +1;//next pg packet addr				
						
				//===========================
				target_pkt.offset = offset;
				target_pkt.word_en= badworden;	
				target_word_cnts = calculate_word_cnts(target_pkt.word_en);
				//===========================			
				WriteState=EFUSE_PG_STATE_HEADER;	
				repeat_times++;
				if(repeat_times>EFUSE_REPROG_THRESHOLD){
					bContinual = _FALSE;
					bResult = _FALSE;
				}
			}
		}
	}

	return bResult;
}
//------------------------------------------------------------------------------
u8 rtw_efuse_access(_adapter *padapter, u8 bRead, u16 start_addr, u16 cnts, u8 *data)
{
	int i = 0;
	u8 res;

	if (start_addr > padapter->eeprompriv.efuse_phy_max_size)
		return _FALSE;

	if ((bRead == _FALSE) && ((start_addr + cnts) > padapter->eeprompriv.efuse_phy_max_size))
		return _FALSE;

	if ((_FALSE == bRead) && (rtw_efuse_reg_init(padapter) == _FALSE))
		return _FALSE;

	//-----------------e-fuse one byte read / write ------------------------------	
	for (i = 0; i < cnts; i++) {
		if ((start_addr + i) > padapter->eeprompriv.efuse_phy_max_size) {
			res = _FALSE;
			break;
		}
		res = efuse_one_byte_rw(padapter, bRead, start_addr + i, data + i);
//		RT_TRACE(_module_rtl871x_mp_ioctl_c_,_drv_err_,("==>rtw_efuse_access addr:0x%02x value:0x%02x\n",data+i,*(data+i)));
		if ((_FALSE == bRead) && (_FALSE == res)) break;
	}

	if (_FALSE == bRead) rtw_efuse_reg_uninit(padapter);

	return res;
}	
//------------------------------------------------------------------------------
u8 rtw_efuse_map_read(_adapter *padapter, u16 addr, u16 cnts, u8 *data)
{
	u8 offset, ret = _TRUE;
	u8 pktdata[EFUSE_PGPKT_DATA_SIZE];
	int i, idx;

	if ((addr + cnts) > EFUSE_MAX_LOGICAL_SIZE)
		return _FALSE;

	if ((efuse_is_empty(padapter, &offset) == _TRUE) && (offset == _TRUE)) {
		for (i = 0; i < cnts; i++)
			data[i] = 0xFF;
		return ret;
	}

	offset = (addr >> 3) & 0xF;
	ret = rtw_efuse_pg_packet_read(padapter, offset, pktdata);
	i = addr & 0x7;	// pktdata index
	idx = 0;	// data index

	do {
		for (; i < EFUSE_PGPKT_DATA_SIZE; i++) {
			data[idx++] = pktdata[i];
			if (idx == cnts) return ret;
		}

		offset++;
//		if (offset > 0xF) break; // no need to check
		if (rtw_efuse_pg_packet_read(padapter, offset, pktdata) == _FALSE)
			ret = _FALSE;
		i = 0;
	} while (1);

	return ret;
}
//------------------------------------------------------------------------------
u8 rtw_efuse_map_write(_adapter* padapter, u16 addr, u16 cnts, u8 *data)
{
	u8 offset, word_en, empty;
	u8 pktdata[EFUSE_PGPKT_DATA_SIZE], newdata[EFUSE_PGPKT_DATA_SIZE];
	int i, j, idx;

	if ((addr + cnts) > EFUSE_MAX_LOGICAL_SIZE)
		return _FALSE;

	// check if E-Fuse Clock Enable and E-Fuse Clock is 40M
	//empty = rtw_read8(padapter, EFUSE_CLK_CTRL);
	//if (empty != 0x03) {
	//	RT_TRACE(_module_rtl8712_efuse_c_, _drv_err_,("rtw_efuse_map_write: EFUSE_CLK_CTRL=0x%2x != 0x03", empty));
	//	return _FALSE;
	//}

	if (efuse_is_empty(padapter, &empty) == _TRUE) {
		if (_TRUE == empty)
			_rtw_memset(pktdata, 0xFF, EFUSE_PGPKT_DATA_SIZE);
	} else
		return _FALSE;

	offset = (addr >> 3) & 0xF;
	if (empty == _FALSE)
		if (rtw_efuse_pg_packet_read(padapter, offset, pktdata) == _FALSE)
			return _FALSE;

	word_en = 0xF;
	_rtw_memset(newdata, 0xFF, EFUSE_PGPKT_DATA_SIZE);
	i = addr & 0x7;	// pktdata index
	j = 0;		// newdata index
	idx = 0;	// data index

	if (i & 0x1) {
		// odd start
		if (data[idx] != pktdata[i]) {
			word_en &= ~BIT(i >> 1);
			newdata[j++] = pktdata[i - 1];
			newdata[j++] = data[idx];
		}
		i++;
		idx++;
	}
	do {
		for (; i < EFUSE_PGPKT_DATA_SIZE; i += 2) {
			if ((cnts - idx) == 1) {
				if (data[idx] != pktdata[i]) {
					word_en &= ~BIT(i >> 1);
					newdata[j++] = data[idx];
					newdata[j++] = pktdata[1 + 1];
				}
				idx++;
				break;
			} else {
				if ((data[idx] != pktdata[i]) || (data[idx+1] != pktdata[i+1])) {
					word_en &= ~BIT(i >> 1);
					newdata[j++] = data[idx];
					newdata[j++] = data[idx + 1];
				}
				idx += 2;
			}
			if (idx == cnts) break;
		}

		if (word_en != 0xF)
			if (rtw_efuse_pg_packet_write(padapter, offset, word_en, newdata) == _FALSE)
				return _FALSE;

		if (idx == cnts) break;

		offset++;
		if (empty == _FALSE)
			if (rtw_efuse_pg_packet_read(padapter, offset, pktdata) == _FALSE)
				return _FALSE;
		i = 0;
		j = 0;
		word_en = 0xF;
		_rtw_memset(newdata, 0xFF, EFUSE_PGPKT_DATA_SIZE);
	} while (1);

	return _TRUE;
}
//------------------------------------------------------------------------------

