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
        for(unsigned k=0;k<PV_EXTRA_COUNT;k++)assert(pv_extra[l][k]&&*pv_extra[l][k]);
        for(unsigned k=0;k<PV_DYNAMIC_COUNT;k++)assert(pv_dynamic[l][k]&&*pv_dynamic[l][k]);
        for(unsigned k=0;k<PV_SESSION_TEXT_COUNT;k++) {
            assert(pv_session_text[l][k]&&*pv_session_text[l][k]);
            assert(!strcmp(pv_translate(l,pv_session_text[0][k]),pv_session_text[l][k]));
        }
        assert(!strcmp(pv_translate(l,"Game title 123"),"Game title 123"));
    }
    assert(!strcmp(pv_translate(2,"Back"),"Voltar"));
    assert(!strcmp(pv_translate(2,"Loading cover"),"Carregando capa"));
    assert(!strcmp(pv_translate(2,"X  Select     O  Back"),"X  Selecionar     O  Voltar"));
    assert(!strcmp(pv_translate(2,"Search: %s"),"Busca: %s"));
    assert(!strcmp(pv_translate(2,"Unsupported regions: Off"),"Regiões sem suporte: Desativado"));
    assert(!strcmp(pv_translate(2,"Filter: %s"),"Filtro: %s"));
    assert(!strcmp(pv_translate(2,"Browse / X Play / Triangle Filter / O Back"),"Navegar / X Jogar / Triângulo Filtro / O Voltar"));
    assert(!strcmp(pv_translate(2,"Cover unavailable"),"Capa indisponível"));
    assert(!strcmp(pv_translate(2,"No cover"),"Sem capa"));
    assert(!strcmp(pv_translate(2,""),""));
    assert(pv_translate(2,NULL)==NULL);
    assert(!strcmp(pv_translate(2,"Finding a cloud server. Circle cancels."),"Buscando servidor. Círculo cancela."));
    assert(!strcmp(pv_translate(2,"On"),"Sim"));
    assert(!strcmp(pv_translate(2,"Forget saved account"),"Esquecer conta salva"));
    for(unsigned l=0;l<PV_LANG_COUNT;l++) {
        const char *p=pv_session_text[l][11];
        unsigned placeholders=0;
        while((p=strchr(p,'%'))!=NULL){assert(p[1]=='s');++placeholders;p+=2;}
        assert(placeholders==3);
    }
    assert(!strcmp(pv_translate(2,"D-pad Up"),"Direcional cima"));
    assert(!strcmp(pv_translate(2,"Rear touch top-left"),"Toque traseiro sup. esquerdo"));
    puts("19 locale tables validated");return 0;
}
