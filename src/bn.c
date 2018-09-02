#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>

#include "bn.h"

char* int_dvd(char* in, int *rmn);
char* int_add(char* a, char* b);
int bn_cmp(bn_t a, bn_t b);

struct bn {
  int bn_len;
  int bn_size;
  int bn_sign;
  uint16_t *bn_data;
};

/* allocate an empty bn_t */
bn_t bn_alloc(void) {
  bn_t bn = (bn_t)malloc(sizeof(struct bn));
  if (bn == NULL)
    return NULL;
  bn->bn_data = (uint16_t *)calloc(1, sizeof(uint16_t));
  if (bn->bn_data == NULL) {
    free(bn);
    return NULL;
  }
  bn->bn_len = 0;
  bn->bn_size = 1;
  bn->bn_sign = 1;
  return bn;
}

void bn_free(bn_t bn){
    free(bn->bn_data);
    bn->bn_data = NULL;
    free(bn);
}

/*
 * construct a bn_t from a string
 * return 0 if success, -1 if fail
*/
int bn_fromString(bn_t bn, const char *s){
    int sign = 1;
    char *strnum;
    if (bn == NULL)
        return -1;

    if(s[0] == '-'){
        sign = -1;
        strnum = strstr(s, "-");
        strnum = strnum + 1;
    }else{
        strnum = (char*)s;
    }
     char *temp = strnum;

    bn->bn_data = (uint16_t*)calloc(strlen(s)/4 , sizeof(uint16_t));
    int i = 0;
    while(1){
        int rm = 0;

        temp = int_dvd(temp, &rm);

        bn->bn_data[i] = rm;

        i++;
        if(atoi(temp)<65536 && strlen(temp)<6){
            if(atoi(temp) == 0){
                i--;
                break;
            }else{
                bn->bn_data[i] = atoi(temp);
                break;
            }
        }
    }


    int k=0;
    for(k=0; k<i+1; k++){
        if(bn->bn_data[k] > 0 && bn->bn_data[k] < 65536){
        }else{
            break;
        }
    }
    free(temp);
    bn->bn_len = k;
    bn->bn_size = k;
    bn->bn_sign = sign;
    return 0;
}

int bn_cmp(bn_t a, bn_t b){
    if(a->bn_len > b->bn_len){
        return 1;
    }else if(a->bn_len < b->bn_len){
        return -1;
    }else{
        for(int i=0; i<a->bn_len; i++){
            if(a->bn_data[i] > b->bn_data[i]){
                return 1;
            }else if(a->bn_data[i] < b->bn_data[i]){
                return -1;
            }
        }
    }
    return 0;
}

/* covert a 10 based number to 65536 based number
 * the remain of the division is a digit number
*/
char* int_dvd(char* in, int *rmn){
    int alen = strlen(in);
    int *sbj;

    sbj = (int*)calloc(alen+5, sizeof(int));

    for(int i=0; i<alen; i++){
        char temp;
        temp = in[i];
        sbj[i] = temp-'0';

    }

    int *res = (int*)calloc(alen, sizeof(int));
    *rmn = 0;
    for(int i=0; i<alen; i++){
        res[i] = sbj[i] / 65536;
        *rmn = sbj[i]%65536;

        sbj[i+1] = *rmn * 10 + sbj[i+1];
    }

    char *ans;
    ans = (char*)calloc(alen+1, sizeof(char));
    int off = 0;
    while(res[off] == 0){
        off++;
    }

    for(int i=0; i<alen; i++){
        ans[i] = res[i+off] + '0';
    }
    ans[alen-off] = '\0';
    free(res);
    return ans;
}

char* int_add(char* a, char* b){
    int l1 = strlen(a);
    int l2 = strlen(b);
    int lr = l1>l2?l1:l2;
    int n1[lr];
    int n2[lr];
    /* uniform number of digits */
    for(int i=0; i<lr; i++){
        n1[i] = 0;
        n2[i] = 0;
    }

    for(int i=0; i<l1; i++){
        n1[i+lr-l1] = a[i]-'0';
    }

    for(int i=0; i<l2; i++){
        n2[i+lr-l2] = b[i]-'0';
    }
    lr++;

    /* school method additon */
    int nr[lr+1];
    int c = 0;
    for(int i=lr-2; i>=0; i--){
        nr[i+1] = (n1[i] + n2[i] + c);
        if(nr[i+1] >= 10){
            nr[i+1] = nr[i+1] % 10;
            c = 1;
        }else{
            c = 0;
        }
    }
    if(c == 1){
        nr[0] = 1;
    }else{
        nr[0] = 0;
    }

    /* remove leading 0s */
    int off = 0;
    while(1){
        if(nr[off] != 0){
            break;
        }
        off++;
    }

    char *res;
    res =(char*)calloc(lr-off+10, sizeof(char));
    for(int i=0; i<lr-off; i++){
        res[i] = nr[i+off]+'0';
    }
    res[lr-off] = '\0';

    return res;
}


/* add two bn_t
 * put result in result
 * return 0 if success
 */
int bn_add(bn_t result, bn_t a, bn_t b){
    int asign = a->bn_sign;
    int bsign = b->bn_sign;
    int rsign;
    a->bn_sign = 1;
    b->bn_sign = 1;
    char nouse[1];
    int alen = bn_toString(a, nouse, 1);
    int blen = bn_toString(b, nouse, 1);
    int len = alen>blen?alen:blen;
    len = len + 10;
    char alpha[len];
    char beta[len];
    bn_toString(a, alpha, len);
    bn_toString(b, beta, len);

    char *res = calloc(len+2, sizeof(char));

    if(asign == bsign){
        res = int_add(alpha, beta);
        rsign = asign;
    }else if(asign != bsign){
        if(bn_cmp(a,b) >= 0){
            //res = int_sub(alpha, beta);
            rsign = asign;
        }else{
            //res = int_sub(beta, alpha);
            rsign = bsign;
        }
    }

    bn_fromString(result, res);
    result->bn_sign = rsign;
    free(res);
    return 1;
}

/* convert bn_t to string */
int bn_toString(bn_t bn, char *buf, int buflen) {
  bn_t dbn = todec(bn);

  if (dbn == NULL)
    return -1;
  int dlen = dbn->bn_len;
  uint16_t *data = dbn->bn_data;

  int requiredlen;
  if (dlen == 0)
    requiredlen = 2;
  else
    requiredlen  = 2 + (bn->bn_sign < 0) + (dlen - 1) * 4 +
	(data[dlen-1] > 999) +
	(data[dlen-1] > 99) +
	(data[dlen - 1] > 9);

  if (requiredlen > buflen) {
    bn_free(dbn);
    return requiredlen;
  }

  char *p = buf;

  if (dlen == 0) {
    *p++ = '0';
  } else {
    if (bn->bn_sign < 0)
      *p++ = '-';
    dlen--;
    if (data[dlen] > 999)
      *p++ = '0' + (data[dlen] / 1000) % 10;
    if (data[dlen] > 99)
      *p++ = '0' + (data[dlen] / 100) % 10;
    if (data[dlen] > 9)
      *p++ = '0' + (data[dlen] / 10) % 10;
    *p++ = '0' + data[dlen] % 10;
    while (dlen--) {
      *p++ = '0' + (data[dlen] / 1000) % 10;
      *p++ = '0' + (data[dlen] / 100) % 10;
      *p++ = '0' + (data[dlen] / 10) % 10;
      *p++ = '0' + (data[dlen] / 1) % 10;
    }
  }
  *p = '\0';

  bn_free(dbn);


  return 0;
}

bn_t todec(bn_t bn) {
  int binlen = bn_reallen(bn);
  int declen = ((binlen + 3)/4) * 5;
  bn_t dbn = bn_alloc();
  if (dbn == NULL)
    return NULL;
  bn_resize(dbn, declen);
  for (int i = binlen; i--; ) {
    dbn_push(dbn, bn->bn_data[i] >> 8);
    dbn_push(dbn, bn->bn_data[i] & 0xFF);
  }

  return dbn;
}

void dbn_push(bn_t bn, uint8_t data) {
  uint32_t carry = data;
  for (int j = 0; j < bn->bn_len; j++) {
    carry += bn->bn_data[j] * 256;
    bn->bn_data[j] = carry % 10000;
    carry = carry / 10000;
  }
  if (carry != 0)
    bn->bn_data[bn->bn_len++] = carry;
}

int bn_reallen(bn_t bn) {
  int l = bn->bn_len;
  while (l-- > 0) {
    if (bn->bn_data[l] != 0)
      return l+1;
  }
  return 0;
}

/* utility function */
int bn_resize(bn_t bn, int size) {
  if (size <= bn->bn_size)
    return 0;
  uint16_t *data = (uint16_t *)realloc(bn->bn_data, size * sizeof(uint16_t));
  if (data == NULL)
    return -1;
  for (int i = bn->bn_size; i < size; i++)
    data[i] = 0;
  bn->bn_data = data;
  bn->bn_size = size;
  return 1;
}
