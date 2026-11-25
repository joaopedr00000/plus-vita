// SPDX-License-Identifier: AGPL-3.0-only
#include "../src/app/i18n.h"
#include <assert.h>
#include <stdio.h>
int main(void){
    assert(PV_LANG_COUNT==19);
    assert(pv_language(19)==0);
    for(unsigned l=0;l<PV_LANG_COUNT;l++){
        assert(pv_locale_names[l]&&*pv_locale_names[l]);
        for(unsigned k=0;k<PV_TEXT_COUNT;k++)assert(pv_text[l][k]&&*pv_text[l][k]);
        assert(!strcmp(pv_translate(l,"Game title 123"),"Game title 123"));
    }
    assert(!strcmp(pv_translate(2,"Back"),"Voltar"));
    puts("19 locale tables validated");return 0;
}
