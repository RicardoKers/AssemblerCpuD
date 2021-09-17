#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct tokenList {
    char token[50];
    int type;
    int addr;
    struct tokenList* next;
};

void attach(struct tokenList** head, char *tk, int type)
{
    struct tokenList* newNode = (struct tokenList*) malloc(sizeof(struct tokenList));
    struct tokenList *last = *head;
    newNode->type=type;
    strcpy(newNode->token,tk);
    newNode->addr=0;
    newNode->next = NULL;
    if (*head == NULL)
    {
        *head= newNode;
        return;
    }

    while (last->next != NULL)
    {
        last = last->next;
    }
    last->next = newNode;
}

void print(struct tokenList *list)
{
    while(list!=NULL)
    {
        printf("%d ",list->type);
        printf("%s ",list->token);
        printf("%d\n",list->addr);
        list=list->next;
    }
}

int stractTokens(struct tokenList** tk, FILE* inPt)
{
    char text[50];
    int ptText=0;
    int c=0;
    while(!feof(inPt))
    {
        c=getc(inPt);
        if(c >= 65 && c <= 90) c = c + 32;
        text[ptText]=c;
        if(c==';')
        {
            while(c>=' ')
            {
                c=getc(inPt);
            }
            ptText=0;
            c=0;
        }
        if((c<=' '||c==','||c==':')&&ptText>0)
        {
            if(c==':') ptText++;
            text[ptText]=0;
            attach(tk, text, 0);
            ptText=0;
            c=0;
        }
        if(c>' ') ptText++;
    }
    return(0);
}

int analizeTokens(struct tokenList** list)
{
    char insts[10][10]={{"mov"},{"wr"},{"rd"},{"nop"},{"xx"},{"xx"},{"xx"},{"xx"},{"xx"},{"xx"}}; // 1 byte instructions
    int numIsnt=10;
    char dblInsts[10][10]={{"ld"},{"jmp"},{"jiz"},{"jin"},{"xx"},{"xx"},{"xx"},{"xx"},{"xx"},{"xx"}}; // 2 bytes instructions
    int numDblIsnt=10;
    char regs[10][5]={{"a"},{"b"},{"ar"},{"pc"},{"xx"}};
    int numRegs=5;
    char commands[10][5]={{"byte"},{"word"},{"xx"},{"xx"},{"xx"}};
    int numCommands=5;
    struct tokenList* temp = *list;
    while(temp!=NULL)
    {
        if(temp->token[strlen(temp->token)-1]==':')
        {
            temp->type=1; // label
        }
        else
        {
            for(int i=0; i<numIsnt; i++)
            {
                if(strcmp(insts[i],temp->token)==0)
                {
                    temp->type=2; // instruction
                }
            }
            for(int i=0; i<numRegs; i++)
            {
                if(strcmp(regs[i],temp->token)==0)
                {
                    temp->type=3; // register
                }
            }
            if(temp->token[0]>='0'&&temp->token[0]<='9')
            {
                temp->type=4; // number
            }
            for(int i=0; i<numDblIsnt; i++)
            {
                if(strcmp(dblInsts[i],temp->token)==0)
                {
                    temp->type=5; // 2 bytes instruction
                }
            }
            for(int i=0; i<numCommands; i++)
            {
                if(strcmp(commands[i],temp->token)==0)
                {
                    if(strcmp("byte",temp->token)==0)
                    {
                        temp->type=6; // command byte
                        temp=temp->next;
                        temp->type=7; // data addr
                    }
                    if(strcmp("word",temp->token)==0)
                    {
                        temp->type=8; // command word
                        temp=temp->next;
                        temp->type=7; // data addr
                    }
                }
            }
        }
        temp=temp->next;
    }
    return(0);
}

int stractLabels(struct tokenList* list)
{
    int addr=0; // prom
    int ramAddr=0; // data
    while(list!=NULL)
    {
        if(list->type==2)
        {
            addr++; // instruction
        }
        if(list->type==5)
        {
            addr+=2; // 2 bytes instruction
        }
        if(list->type==1)
        {
            list->addr=addr; // label
        }
        if(list->type==6) // byte
        {
            list=list->next;
            list->addr=ramAddr; // byte
            ramAddr++;
        }
        if(list->type==8) // word
        {
            list=list->next;
            list->addr=ramAddr; // word
            ramAddr+=2;
        }
        list=list->next;
    }
    return(0);
}

int getNumber(char* txt)
{
    int ret;
    if(txt[1]=='x'||txt[2]=='x') //hexadecimal
    {
        sscanf(txt,"%x",&ret);
    }
    else
    {
        ret=atoi(txt);
    }
    return(ret);
}

int compile(struct tokenList* list, FILE* arqOut)
{
    int addr=0; // PROM
    int ramAddr=0; // data
    struct tokenList* tmp = list;
    struct tokenList* listH = list;
    fprintf(arqOut,"v2.0 raw\n");
    while(list!=NULL)
    {
        if(list->type==2||list->type==5) //instruction
        {
            addr++;
            // nop
            if(strcmp(list->token,"nop")==0)    // nop
            {
                fprintf(arqOut,"0\n");
            }
            // ld
            if(strcmp(list->token,"ld")==0)    // ld
            {
                addr++;
                list=list->next;
                if(strcmp(list->token,"a")==0)    // ld a,val
                {
                    list=list->next;
                    fprintf(arqOut,"1\n");
                    fprintf(arqOut,"%X\n",getNumber(list->token));
                }
                if(strcmp(list->token,"b")==0)    // ld b,val
                {
                    list=list->next;
                    fprintf(arqOut,"2\n");
                    fprintf(arqOut,"%X\n",getNumber(list->token));
                }
                if(strcmp(list->token,"ar")==0)    // ld ar,val
                {
                    list=list->next;
                    fprintf(arqOut,"3\n");
                    if(list->token[0]>='0'&&list->token[0]<='0') // number
                    {
                        fprintf(arqOut,"%X\n",getNumber(list->token));
                    }
                    else // byte or word addr
                    {
                        tmp=listH;
                        while(tmp!=NULL) // find text for addr
                        {
                            if(tmp->type==7)
                            {
                                if(strcmp(tmp->token,list->token)==0)
                                {
                                    printf("RAM %s = %d\n",tmp->token,tmp->addr);
                                    fprintf(arqOut,"%X\n",tmp->addr);
                                    break;
                                }
                            }
                            tmp=tmp->next;
                        }
                    }
                }
            }
            // mov
            if(strcmp(list->token,"mov")==0)    // mov
            {
                list=list->next;
                if(strcmp(list->token,"b")==0)    // mov b,
                {
                    list=list->next;
                    if(strcmp(list->token,"a")==0)    // mov b,a
                    {
                        fprintf(arqOut,"4\n");
                    }
                    /*if(strcmp(list->token,"ar")==0)    // mov b,ar
                    {
                        fprintf(arqOut,"xxxx\n");
                    }*/
                }
                if(strcmp(list->token,"a")==0)    // mov a,
                {
                    list=list->next;
                    if(strcmp(list->token,"b")==0)    // mov a,b
                    {
                        fprintf(arqOut,"5\n");
                    }
                    /*if(strcmp(list->token,"ar")==0)    // mov a,ar
                    {
                        fprintf(arqOut,"xxxx\n");
                    }*/
                }
                if(strcmp(list->token,"ar")==0)    // mov ar,
                {
                    list=list->next;
                    if(strcmp(list->token,"b")==0)    // mov ar,a
                    {
                        fprintf(arqOut,"6\n");
                    }
                    if(strcmp(list->token,"b")==0)    // mov ar,b
                    {
                        fprintf(arqOut,"7\n");
                    }
                }
            }
            // wr
            if(strcmp(list->token,"wr")==0)    //wr
            {
                list=list->next;
                if(strcmp(list->token,"a")==0)    // wr a
                {
                    fprintf(arqOut,"8\n");
                }
                if(strcmp(list->token,"b")==0)    // wr b
                {
                    fprintf(arqOut,"9\n");
                }
            }
            // rd
            if(strcmp(list->token,"rd")==0)    // rd
            {
                list=list->next;
                if(strcmp(list->token,"a")==0)    // rd a
                {
                    fprintf(arqOut,"A\n");
                }
                if(strcmp(list->token,"b")==0)    // rd b
                {
                    fprintf(arqOut,"B\n");
                }
            }
            // jmp
            if(strcmp(list->token,"jmp")==0)    // jmp
            {
                addr++;
                list=list->next;
                if(list->token[0]>='0'&&list->token[0]<='9')    // jmp <addr>
                {
                    fprintf(arqOut,"C\n"); //12
                    fprintf(arqOut,"%X\n",getNumber(list->token));
                }
                else // jmp label
                {
                    tmp=listH;
                    while(tmp!=NULL) // find label addr
                    {
                        //printf("(%s)\n",tmp->token);
                        char tmpCmp[50];
                        strcpy(tmpCmp,list->token);
                        tmpCmp[strlen(tmpCmp)+1]=0; // add '\0'
                        tmpCmp[strlen(tmpCmp)]=':'; // add '\0'
                        if(strcmp(tmp->token,tmpCmp)==0)
                        {
                            printf("label %s = %d\n",tmp->token,tmp->addr);
                            fprintf(arqOut,"C\n");
                            fprintf(arqOut,"%X\n",tmp->addr);
                            break;
                        }
                        tmp=tmp->next;
                    }
                }
            }
            // jiz
            if(strcmp(list->token,"jiz")==0)    // jiz
            {
                addr++;
                list=list->next;
                if(strcmp(list->token,"a")==0)
                {
                    list=list->next;
                    if(list->token[0]>='0'&&list->token[0]<='9')    // jiz a,<addr>
                    {
                        fprintf(arqOut,"D\n"); // 13
                        fprintf(arqOut,"%X\n",getNumber(list->token));
                    }
                    else // jiz a,label
                    {
                        tmp=listH;
                        while(tmp!=NULL) // find label addr
                        {
                            //printf("(%s)\n",tmp->token);
                            char tmpCmp[50];
                            strcpy(tmpCmp,list->token);
                            tmpCmp[strlen(tmpCmp)+1]=0; // add '\0'
                            tmpCmp[strlen(tmpCmp)]=':'; // add '\0'
                            if(strcmp(tmp->token,tmpCmp)==0)
                            {
                                printf("label %s = %d\n",tmp->token,tmp->addr);
                                fprintf(arqOut,"D\n");
                                fprintf(arqOut,"%X\n",tmp->addr);
                                break;
                            }
                            tmp=tmp->next;
                        }
                    }
                }
            }
        }

        list=list->next;
    }
    printf("PROM = %d bytes",addr);
    return(0);
}

int main()
{
    FILE* arqIn;
    FILE* arqOut;
    struct tokenList* tokens = NULL;

    printf("\nAssembler V0.1\n");

    arqIn = fopen("in.txt", "r");

    if(arqIn == NULL) // testa se o arquivo foi aberto com sucesso
    {
        printf("\n\nImpossivel abrir o arquivo de entrada!\n\n");
        return 0;
    }

    arqOut = fopen("out.hex", "w");

    if(arqOut == NULL) // testa se o arquivo foi aberto com sucesso
    {
        printf("\n\nImpossivel abrir o arquivo de saida!\n\n");
        return 0;
    }

    stractTokens(&tokens, arqIn);
    analizeTokens(&tokens);
    stractLabels(tokens);
    print(tokens);
    compile(tokens, arqOut);

    fclose(arqIn);
    fclose(arqOut);
    return 0;
}
