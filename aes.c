#include<string.h>
#include<assert.h>
#include "aes.h"
#include "conf.c"

#ifndef GET_UINT32

//�궨��һ����λ�ٰ�����b���ĸ�ֵ��ֵ��n
#define GET_UINT32(n,b,i) do { \
	(n) = ((uint32_t)(b)[(i)    ]      ) \
		| ((uint32_t)(b)[(i) + 1] <<  8) \
		| ((uint32_t)(b)[(i) + 2] << 16) \
		| ((uint32_t)(b)[(i) + 3] << 24);\
} while(0)
#endif

#define ROTL8(x) (((x) << 24) | ((x) >> 8))
#define ROTL16(x) (((x) << 16) | ((x) >> 16))
#define ROTL24(x) (((x) << 8) | ((x) >> 24))

//s���滻ÿһλ
#define SUB_WORD(x) (((uint32_t)S_BOX[(x)&0xFF]) \
	| ((uint32_t)S_BOX[((x) >>  8)&0xFF] << 8) \
	| ((uint32_t)S_BOX[((x) >> 16)&0xFF] << 16) \
	| ((uint32_t)S_BOX[((x) >> 24)&0xFF] << 24) \
	)

#define sub_bytes(state) _sub_bytes(state, S_BOX)
#define inv_sub_bytes(state) _sub_bytes(state, INV_S_BOX)
#define shift_rows(state) _shift_rows(state, ROTL8, ROTL16, ROTL24)
#define inv_shift_rows(state) _shift_rows(state, ROTL24, ROTL16, ROTL8)
#define mix_columns(state) _mix_columns(state, MIX)
#define inv_mix_columns(state) _mix_columns(state, INV_MIX)

//����λ
#define _shift_rows(state, OP1, OP2, OP3) do { \
	transport(state); \
	*(uint32_t *)(state+4) = OP1(*(uint32_t *)(state+4)); \
	*(uint32_t *)(state+8) = OP2(*(uint32_t *)(state+8)); \
	*(uint32_t *)(state+12) = OP3(*(uint32_t *)(state+12)); \
	transport(state); \
} while(0)


/**************************************************************************** 
Description:          ����λ������(transport) 
Input parameters:        
                        uint8_t state:��Ҫ����λ������                                       
Output parameters:       
                        uint8_t state:����λ�Ľ��
                         
Returned value:      
                        NULL
Created by:          
                        ������ (2017-09-01) 
Modified by:             
                        NULL 
****************************************************************************/  
static void transport(uint8_t state[BLOCK_SIZE])
{
	uint8_t _state[4][4];
	int r,c;

	assert(state != NULL);//�Բ���У��
	
	for (r = 0; r < 4; ++r)
		for (c = 0; c < 4; ++c)
			_state[r][c] = state[(c<<2)+r];
	memcpy(state, _state, sizeof(_state));
}

/**************************************************************************** 
Description:           �������(add_round_key) ����Կ��
Input parameters:        
                        uint8_t state:��Ҫ����������1
                        const uint8_t key : ��Ҫ����������2                      
Output parameters:       
                        uint8_t state:������Ľ��
                         
Returned value:      
                        NULL
Created by:          
                        ������ (2017-09-01) 
Modified by:             
                        NULL 
****************************************************************************/  
static void add_round_key(uint8_t state[BLOCK_SIZE], const uint8_t key[BLOCK_SIZE])
{
	int i;

	assert(state != NULL);//�Բ���У��
	assert(key != NULL);
	
	for (i = 0; i < BLOCK_SIZE; ++i)
		state[i] ^= key[i];
}

/**************************************************************************** 
Description:          S���ֽ��滻(_sub_bytes) 
Input parameters:        
                        uint8_t state:��Ҫ�滻������
                        cuint8_t *BOX : s������                    
Output parameters:       
                        uint8_t state:�滻�������
                         
Returned value:      
                        NULL
Created by:          
                        ������ (2017-09-01) 
Modified by:             
                        NULL 
****************************************************************************/  
static void _sub_bytes(uint8_t state[BLOCK_SIZE], const uint8_t *BOX)
{
	int i;

	assert(state != NULL);//�Բ���У��
	assert(BOX != NULL);
	
	for (i = 0; i < BLOCK_SIZE; ++i)
		state[i] = BOX[state[i]];
}

static uint8_t GF_256_multiply(uint8_t a, uint8_t b)
{
	uint8_t t[8] = { a };
	uint8_t ret = 0x00;
	int i = 0;

	for (i = 1; i < 8; ++i) {
		t[i] = t[i-1] << 1;
		if (t[i-1]&0x80) t[i] ^= 0x1b;
	}
	for (i = 0; i < 8; ++i)
		ret ^= (((b >> i) & 0x01) * t[i]);
	return ret;
}

/**************************************************************************** 
Description:          4*4����ĳ˷�(_mix_columns) 
Input parameters:        
                        uint8_t state:����1
                        uint8_t matrix: ����2        
Output parameters:       
                        uint8_t state:���
                         
Returned value:      
                        NULL
Created by:          
                        ������ (2017-09-01) 
Modified by:             
                        NULL 
****************************************************************************/  
static void _mix_columns(uint8_t state[BLOCK_SIZE], const uint8_t matrix[][4])
{
	uint8_t _state[BLOCK_SIZE] = {0};
	int r,c,i;

	assert(state != NULL);//�Բ���У��
	assert(matrix != NULL);
	
	for (r = 0; r < 4; ++r)
		for (c = 0; c < 4; ++c)
			for (i = 0; i < 4; ++i)
				_state[(c<<2)+r] ^= GF_256_multiply(matrix[r][i], state[(c<<2)+i]);
	memcpy(state, _state, sizeof(_state));
}


/**************************************************************************** 
Description:          AES�����м���ִ���(aes_round) 
Input parameters:        
                        uint8_t state :  ������м�����
                        uint8_t rk:��Կ ������м�����
Output parameters:       
                        uint8_t state :  �������м�����
                         
Returned value:      
                        NULL 
Created by:          
                        ������ (2017-09-01) 
Modified by:             
                        NULL 
****************************************************************************/  
static void aes_round(uint8_t state[BLOCK_SIZE], const uint8_t rk[BLOCK_SIZE])
{
	sub_bytes(state);//s���滻
	shift_rows(state);//����λ
	mix_columns(state);//�л���
	add_round_key(state, rk);//����Կ��
}

/**************************************************************************** 
Description:          AES�����м���ִ���(aes_inv_round) 
Input parameters:        
                        uint8_t state :  ������м�����
                        uint8_t inv_rk:��Կ ������м�����
Output parameters:       
                        uint8_t state :  �������м�����
                         
Returned value:      
                        NULL 
Created by:          
                        ������ (2017-09-01) 
Modified by:             
                        NULL 
****************************************************************************/  
static void aes_inv_round(uint8_t state[BLOCK_SIZE], const uint8_t inv_rk[BLOCK_SIZE])
{
	inv_shift_rows(state);
	inv_sub_bytes(state);
	add_round_key(state, inv_rk);
	inv_mix_columns(state);
}

/**************************************************************************** 
Description:          AES��������ִ���(aes_final_round) 
Input parameters:        
                        uint8_t state :  ������м�����
                        uint8_t rk:��Կ ������м�����
Output parameters:       
                        uint8_t state :  �������м�����
                         
Returned value:      
                        NULL 
Created by:          
                        ������ (2017-09-01) 
Modified by:             
                        NULL 
****************************************************************************/  
static void aes_final_round(uint8_t state[BLOCK_SIZE], const uint8_t rk[BLOCK_SIZE])
{
	sub_bytes(state);//s���滻
	shift_rows(state);//����λ
	add_round_key(state, rk);//����Կ��
}

/**************************************************************************** 
Description:          AES��������ִ���(aes_final_round) 
Input parameters:        
                        uint8_t state :  ������м�����
                        uint8_t inv_rk:��Կ ������м�����
Output parameters:       
                        uint8_t state :  �������м�����
                         
Returned value:      
                        NULL 
Created by:          
                        ������ (2017-09-01) 
Modified by:             
                        NULL 
****************************************************************************/  
static void inv_final_round(uint8_t state[BLOCK_SIZE], const uint8_t inv_rk[BLOCK_SIZE])
{
	inv_shift_rows(state);
	inv_sub_bytes(state);
	add_round_key(state, inv_rk);
}

/**************************************************************************** 
Description:           ������Կ(key_expansion) 
Input parameters:        
                        aes_context *ctx :  ��������Ĳ���(��Կ����)
                        const uint8_t *key:��Կ 
Output parameters:       
                        aes_context *ctx :  ����ת�����ݵĽṹ��
                         
Returned value:      
                        NULL 
Created by:          
                        ������ (2017-09-01) 
Modified by:             
                        NULL 
****************************************************************************/  
static void key_expansion(aes_context *ctx, const uint8_t *key)
{
	uint32_t Nk = ctx->nr - 6;
	uint32_t Ek = (ctx->nr+1)<<2;
	uint32_t *RK = ctx->rk;
	uint32_t i = 0;

	assert(ctx != NULL);
	assert(key != NULL);//����У��

	for(i = 0;i<68;i++)
		RK[i] = 0;
	
	i = 0;
	do
	{
		GET_UINT32(RK[i], key, i<<2);//��4λuchar������λ��һλuint����
	} while(++i < Nk);
	do
	{
		uint32_t t = RK[i-1];
		if ((i % Nk) == 0)
			t = SUB_WORD(ROTL8(t))^RCON[i/Nk -1];//���滻�����
		else if (Nk == 8 && (i % Nk) == 4)
			t = SUB_WORD(t);//ֱ��s���滻
		RK[i] = RK[i-Nk]^t;
	} while(++i < Ek);
}

/**************************************************************************** 
Description:           ��ʼ����Կ�ṹ��(aes_set_key) 
Input parameters:        
                        const uint8_t *key:��Կ
                        uint32_t key_bit:AES��������(128, 192, 256)
Output parameters:       
                        aes_context *ctx : ��ʼ����AES�ṹ��
                         
Returned value:      
                        NULL 
Created by:          
                        ������ (2017-09-01) 
Modified by:             
                        NULL 
****************************************************************************/  
int aes_set_key(aes_context *ctx, const uint8_t *key, uint32_t key_bit)
{
	if (ctx == NULL || key == NULL)
		return PARM_ERROR;
	switch (key_bit)
	{
		case 128: ctx->nr = 10; break;
		case 192: ctx->nr = 12; break;
		case 256: ctx->nr = 14; break;
		default: return PARM_ERROR;
	}
	ctx->rk = ctx->buf;
	key_expansion(ctx, key);//��Կ����
	return SUCCESS;
}

/**************************************************************************** 
Description:           AES���ܺ���(aes_encrypt_block) 
Input parameters:        
                        aes_context *ctx:��������Կ��ʼ����AES�ṹ��
                        uint8_t text:��������
Output parameters:       
                        uint8_t cipher_text : ���ܺ������
                         
Returned value:      
                        SUCCESS  (�ɹ�)
                        other       (ʧ��)
Created by:          
                        ������ (2017-09-01) 
Modified by:             
                        NULL 
****************************************************************************/  
int aes_encrypt_block(aes_context *ctx
	, uint8_t cipher_text[BLOCK_SIZE], const uint8_t text[BLOCK_SIZE])
{
	uint32_t Nr = 0;
	uint32_t *RK = NULL;
	uint8_t *state = NULL;
	uint32_t i;

	if (ctx == NULL || cipher_text == NULL || text == NULL)
		return PARM_ERROR;
	if (ctx->rk != ctx->buf)
		return NOT_INIT_KEY;

	Nr = ctx->nr;
	RK = ctx->rk;
	state = cipher_text;
	memcpy(state, text, BLOCK_SIZE);

	add_round_key(state, (const uint8_t *)RK);//�����������Կ���������(��ʼ����Կ��)
	
	for (i = 1; i < Nr; ++i)
		aes_round(state, (const uint8_t *)(RK + (i<<2)));
	aes_final_round(state, (const uint8_t *)(RK + (Nr<<2)));
	
	return SUCCESS;
}

/**************************************************************************** 
Description:           AES���ܺ���(aes_decrypt_block) 
Input parameters:        
                        aes_context *ctx:��������Կ��ʼ����AES�ṹ��
                        uint8_t cipher_text : ���ܵ�����                       
Output parameters:       
                        uint8_t text:���ܺ�����
                         
Returned value:      
                        SUCCESS  (�ɹ�)
                        other       (ʧ��)
Created by:          
                        ������ (2017-09-01) 
Modified by:             
                        NULL 
****************************************************************************/  
int aes_decrypt_block(aes_context *ctx
	, uint8_t text[BLOCK_SIZE], const uint8_t cipher_text[BLOCK_SIZE])
{
	uint32_t Nr = 0;
	uint32_t *INV_RK = NULL;
	uint8_t *state = NULL;
	uint32_t i;

	if (ctx == NULL || text == NULL || cipher_text == NULL)
		return PARM_ERROR;
	if (ctx->rk != ctx->buf)
		return NOT_INIT_KEY;

	Nr = ctx->nr;
	INV_RK = ctx->rk;
	state = text;
	memcpy(state, cipher_text, BLOCK_SIZE);
	
	add_round_key(state, (const uint8_t *)(INV_RK + (Nr<<2)));
	
	for (i = Nr-1; i > 0; --i)
		aes_inv_round(state, (const uint8_t *)(INV_RK + (i<<2)));
	inv_final_round(state, (const uint8_t *)INV_RK);
	
	return SUCCESS;
}
