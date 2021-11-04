#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
token types>
comments> ; this is a comment

instructions>
    1 byte instruction: mov a, b
    2 byte instruction: ld a, 0x12

label> reset:

registers> a

commands> byte

--------------
types>
type=0 not valid
type=1 label
type=2 1 byte instruction
type=3 register
type=4 number
type=5 2 bytes instructions
type=6 command byte
type=7 data addr
type=8 command word
type=9 command define
type=10 define name
type=11 command const
type=12 const name
*/

unsigned char PROM[1024];
int addr=0; // PROM

struct tokenList {
    char token[50];
    int type;
    int addr;
    int lineNumber;
    struct tokenList* next;
};

void attach(struct tokenList** head, char *tk, int type, int line)
{
    struct tokenList* newNode = (struct tokenList*) malloc(sizeof(struct tokenList));
    struct tokenList *last = *head;
    newNode->type=type;
    newNode->lineNumber=line;
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
        printf("type=%d ",list->type);
        printf("token=%s ",list->token);
        printf("addr=%d",list->addr);
        printf(" (line = %d)\n",list->lineNumber);
        list=list->next;
    }
}

int extractTokens(struct tokenList** tk, FILE* inPt)
{
    char text[50];
    int ptText=0;
    int c=0;
    int lineCounter=1;
    while(!feof(inPt))
    {
        c=getc(inPt);
        if(c >= 65 && c <= 90) c = c + 32;
        text[ptText]=c;
        if(c==';')
        {
            while(c>=' '||c=='\t')
            {
                c=getc(inPt);
            }
            if(c=='\n') lineCounter++;
            ptText=0;
            c=0;
        }
        if((c<=' '||c==','||c==':')&&ptText>0)
        {
            if(c==':') ptText++;
            text[ptText]=0;
            attach(tk, text, 0, lineCounter);
            if(c=='\n') lineCounter++;
            ptText=0;
            c=0;
        }
        else
        {
            if(c=='\n') lineCounter++;
        }

        if(c>' ') ptText++;
    }
    return(0);
}

int analizeTokens(struct tokenList** list)
{
    //char insts[30][10]={{"nop"},{"ld"},{"mov"},{"wr"},{"rd"},{"in"},{"out"},{"inc"},{"jmp"},{"jiz"},{"jie"},{"jig"},{"jis"},{"jin"},{"jic"},{"set"},{"clr"},{"not"},{"or"},{"and"},{"xor"},{"add"},{"sub"},{"dec"},{"lsl"},{"slr"},{"swap"},{"xxxx"},{"xxxx"},{"xxxx"}}; //instructions
    char insts[24][10]={{"nop"},{"ld"},{"mov"},{"wr"},{"rd"},{"in"},{"out"},{"jmp"},{"jiz"},{"jie"},{"jig"},{"jis"},{"jin"},{"jic"},{"set"},{"clr"},{"not"},{"or"},{"and"},{"xor"},{"add"},{"sub"},{"lsl"},{"lsr"}}; //instructions
    int numIsnt=24;
    //char regs[6][10]={{"a"},{"b"},{"ar"},{"ar+"},{"pc"},{"c"}};
    char regs[3][10]={{"a"},{"b"},{"c"}};
    int numRegs=3;
    char commands[4][10]={{"byte"},{"word"},{"define"},{"const"}};
    int numCommands=4;
    int ret=0;
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
            // type 5 reserved for 2 bytes instructions
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
                    if(strcmp("define",temp->token)==0)
                    {
                        temp->type=9; // command word
                    }
                    if(strcmp("const",temp->token)==0)
                    {
                        temp->type=11; // command word
                    }
                }
            }
        }
        temp=temp->next;
    }
    return(ret);
}

int extractDataAddr(struct tokenList* list)
{
    int ramAddr=0; // data
    while(list!=NULL)
    {
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

int extractNumber(char *txt)
{
    int num;
    if(txt[1]=='x'||txt[1]=='X') // hex
    {
        sscanf(txt, "%x", &num);
    }
    else
    {
        sscanf(txt, "%d", &num);
    }
    return(num);
}


int extractConst(struct tokenList* list)
{
    int ret=0;
    while(list!=NULL)
    {
        if(list->type==11) // constant
        {
            list=list->next;
            if(list->type==0)
            {
                list->type=12; // constant name
            }
            else
            {
                printf("[ERROR Line %d] Invalid constant name (%s)\n",list->lineNumber,list->token);
                ret=1;
            }
            list=list->next;
            if(list->type!=4)
            {
                printf("[ERROR Line %d] Invalid constant value (%s)\n",list->lineNumber,list->token);
                ret=1;
            }
        }
        list=list->next;
    }
    return(ret);
}


int updateConst(struct tokenList* list)
{
    //struct tokenList* hlist = list;
    //struct tokenList* templist = list;
    int addr=0;
    while(list!=NULL)
    {
        if(list->type==2) // 1 byte instruction
        {
            addr++;
        }
        if(list->type==5) // 2 byte instruction
        {
            addr+=2;
        }
        if(list->type==12) // constant name
        {
            list->addr=addr;
            addr++;
        }
        list=list->next;
    }
    return(0);
}

int setConst(struct tokenList* list)
{
    struct tokenList* hlist = list;
    struct tokenList* templist = list;
    while(list!=NULL)
    {
        if(list->type!=12) // not a constant declaration
        {
            templist = hlist;
            while(templist!=NULL)
            {
                if(templist->type==12)
                {
                    if(strcmp(templist->token,list->token)==0)
                    {
                        sprintf(list->token,"%d",templist->addr);
                        list->type=4;
                    }
                }
                templist=templist->next;
            }
        }
        list=list->next;
    }
    return(0);
}

int extractDefines(struct tokenList* list)
{
    int ret=0;
    while(list!=NULL)
    {
        if(list->type==9) // define
        {
            list=list->next;
            if(list->type==0)
            {
                list->type=10; // define name
                if(list->next->type==4)
                {
                    list->addr=extractNumber(list->next->token);
                }
                else
                {
                    printf("[ERROR Line %d] Invalid define value (%s)\n",list->lineNumber,list->next->token);
                    ret=1;
                }
            }
            else
            {
                printf("[ERROR Line %d] Invalid define name (%s)\n",list->lineNumber,list->token);
                ret=1;
            }
        }
        list=list->next;
    }
    return(ret);
}

int setDefines(struct tokenList* list)
{
    struct tokenList* hlist = list;
    struct tokenList* templist = list;
    while(list!=NULL)
    {
        if(list->type!=10) // not a define declaration
        {
            templist = hlist;
            while(templist!=NULL)
            {
                if(templist->type==10)
                {
                    if(strcmp(templist->token,list->token)==0)
                    {
                        sprintf(list->token,"%d",templist->addr);
                        list->type=4;
                    }
                }
                templist=templist->next;
            }
        }
        list=list->next;
    }
    return(0);
}

int setDataAddr(struct tokenList* list)
{
    struct tokenList* temp = list;
    struct tokenList* tempStart = list;

    while(list!=NULL)
    {
        if(list->type==0) // no type definid
        {
            temp = tempStart;
            while(temp!=NULL) //find token in temp
            {
                if(temp->type==7)
                {
                    if(strcmp(list->token,temp->token)==0)
                    {
                        list->type=7;
                        list->addr=temp->addr;
                    }
                }
                temp=temp->next;
            }
        }
        list=list->next;
    }
    return(0);
}

int extract2BytsInstruction(struct tokenList* list) // type = 5
{
    struct tokenList* temp;
    while(list!=NULL)
    {
        if(list->type==2) // instruction
        {
            // ld
            if(strcmp("ld",list->token)==0)
            {
                temp=list->next;
                if(strcmp(temp->next->token,"b")==0 )// constant address
                {
                    list->type=2;
                }
                else
                {
                    list->type=5;
                }
            }
            // wr
            if(strcmp("wr",list->token)==0)
            {
                if(list->next->type==4||list->next->type==7)
                {
                    // wr addr,reg
                    list->type=5;
                }
            }
            // rd
            if(strcmp("rd",list->token)==0)
            {
                temp=list->next;
                if(temp->next->type==4||temp->next->type==7)
                {
                    // rd xx,addr
                    list->type=5;
                }
            }
            // in
            if(strcmp("in",list->token)==0)
            {
                temp=list->next;
                if(temp->next->type==4||temp->next->type==7)
                {
                    // in xx,addr
                    list->type=5;
                }
            }
            // out
            if(strcmp("out",list->token)==0)
            {
                temp=list->next;
                if(temp->next->type==4)
                {
                    // out xx,val
                    list->type=5;
                }
                if(list->next->type==4||list->next->type==7)
                {
                    // out addr,xx
                    list->type=5;
                }
            }
            // jmp
            if(strcmp("jmp",list->token)==0)
            {
                temp=list->next;
                if(strcmp(temp->token,"b")==0 )// constant address
                {
                    list->type=2;
                }
                else
                {
                    list->type=5;
                }
            }
            // jiz
            if(strcmp("jiz",list->token)==0)
            {
                list->type=5;
            }
            // jie
            if(strcmp("jie",list->token)==0)
            {
                list->type=5;
            }
            // jig
            if(strcmp("jig",list->token)==0)
            {
                list->type=5;
            }
            // jis
            if(strcmp("jis",list->token)==0)
            {
                list->type=5;
            }
            // jin
            if(strcmp("jin",list->token)==0)
            {
                list->type=5;
            }
            // jic
            if(strcmp("jic",list->token)==0)
            {
                list->type=5;
            }
        }
        list=list->next;
    }
    return(0);
}

int extractLabels(struct tokenList* list)
{
    int promAddr=0; // prom
    while(list!=NULL)
    {
        if(list->type==2)
        {
            promAddr++; // instruction
            printf("=========== 1 %s \n",list->token);
        }
        if(list->type==5)
        {
            promAddr+=2; // 2 bytes instruction
            printf("=========== 2 %s \n",list->token);
        }
        if(list->type==1)
        {
            list->addr=promAddr; // label
        }
        list=list->next;
    }
    return(0);
}

int setLabels(struct tokenList* list)
{
    struct tokenList* temp = list;
    struct tokenList* tempStart = list;

    char tempStr[50];

    while(list!=NULL)
    {
        if(list->type==0) // no type definid
        {
            temp = tempStart;
            while(temp!=NULL) //find token in temp
            {
                if(temp->type==1)
                {
                    strcpy(tempStr,temp->token);
                    tempStr[strlen(tempStr)-1]=0; // remove ':'
                    if(strcmp(list->token,tempStr)==0)
                    {
                        list->type=1;
                        list->addr=temp->addr;
                    }
                }
                temp=temp->next;
            }
        }
        list=list->next;
    }
    return(0);
}

int errorCheck(struct tokenList* list)
{
    int ret=0; // o=no error
    while(list!=NULL)
    {
        if(list->type==0)
        {
            printf("[ERROR Line %d] Command not recognized (%s)\n",list->lineNumber,list->token);
            ret=1;
        }
        list=list->next;
    }
    return(ret);
}

/*
int compile(struct tokenList* list, FILE* arqOut) // Hneemaan Digital format
{
    int ret=0; // return value, 0=no error
    int tmpInt;
    int tmpAddr;
    int invalidInstruction;
    int instructionFiniched;
    fprintf(arqOut,"v2.0 raw\n");
    while(list!=NULL)
    {
        if(list->type==2||list->type==5) //instruction
        {
            //printf("inst -> %s, %s (l%d)\n",list->token,list->next->token,list->lineNumber);
            addr++;
            invalidInstruction=1;
            instructionFiniched=0;
            // NOP
            if(strcmp(list->token,"nop")==0)
            {
                fprintf(arqOut,"0\n"); // nop
                invalidInstruction=0;
                instructionFiniched=1;
            }
            //LD
            if(strcmp(list->token,"ld")==0 && instructionFiniched==0)
            {
                list=list->next;
                // ld a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // ld a, val
                    if(list->type==4 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"1\n"); // ld a, val
                        fprintf(arqOut,"%X\n",extractNumber(list->token)); // val
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // ld a, ar
                    if(strcmp(list->token,"ar")==0 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"4\n"); // ld a, ar
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // ld a, ar+
                    if(strcmp(list->token,"ar+")==0)
                    {
                        fprintf(arqOut,"6\n"); // ld a, ar+
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
                // ld b,
                if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // ld b, val
                    if(list->type==4)
                    {
                        fprintf(arqOut,"2\n"); // ld b, val
                        fprintf(arqOut,"%X\n",extractNumber(list->token)); // val
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // ld b, ar
                    if(strcmp(list->token,"ar")==0)
                    {
                        fprintf(arqOut,"5\n"); // ld b, ar
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // ld b, ar+
                    if(strcmp(list->token,"ar+")==0)
                    {
                        fprintf(arqOut,"7\n"); // ld b, ar+
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
                // ld ar,
                if(strcmp(list->token,"ar")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    if((list->type==4||list->type==7) && instructionFiniched==0)
                    {
                        // ld ar, addr
                        // bits 8 and 9 of addr inserted into instruction
                        if(list->type==4) // is a number
                        {
                            tmpAddr=extractNumber(list->token);
                        }
                        else // type 7
                        {
                            tmpAddr=list->addr;
                        }
                        tmpInt=tmpAddr;
                        tmpInt=tmpInt&0x300; // extract bits 8 and 9
                        tmpInt=tmpInt>>2; // move bits 8 and 9 to 6 and 7 position
                        tmpInt=tmpInt|3; // insert opcode 3
                        fprintf(arqOut,"%X\n",tmpInt);
                        tmpInt=tmpAddr;
                        tmpInt=tmpInt&0xFF; // extract bits 0 to 7
                        fprintf(arqOut,"%X\n",tmpInt); // val
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
            }
            //MOV
            if(strcmp(list->token,"mov")==0 && instructionFiniched==0)
            {
                list=list->next;
                // mov b,
                if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // mov b, a
                    if(strcmp(list->token,"a")==0)
                    {
                        fprintf(arqOut,"8\n"); // mov b, a
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
                // mov a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // mov a, b
                    if(strcmp(list->token,"b")==0)
                    {
                        fprintf(arqOut,"9\n"); // mov a, b
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
            }
            //WR
            if(strcmp(list->token,"wr")==0 && instructionFiniched==0)
            {
                list=list->next;
                // wr ar,
                if(strcmp(list->token,"ar")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // wr ar, a
                    if(strcmp(list->token,"a")==0)
                    {
                        fprintf(arqOut,"A\n"); // wr ar, a
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // wr ar, b
                    if(strcmp(list->token,"b")==0)
                    {
                        fprintf(arqOut,"B\n"); // wr ar, b
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
                // wr ar+,
                if(strcmp(list->token,"ar+")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // wr ar+, a
                    if(strcmp(list->token,"a")==0)
                    {
                        fprintf(arqOut,"C\n"); // wr ar+, a
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // wr ar+, b
                    if(strcmp(list->token,"b")==0)
                    {
                        fprintf(arqOut,"D\n"); // wr ar+, b
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
                // wr addr,
                if((list->type==4||list->type==7) && instructionFiniched==0)
                {
                    if(list->type==4) // is a number
                    {
                        tmpAddr=extractNumber(list->token);
                    }
                    else // type 7
                    {
                        tmpAddr=list->addr;
                    }
                    list=list->next;
                    // wr addr, a
                    if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                    {
                        // wr addr, a
                        tmpInt=tmpAddr&0x300; // extract bits 8 and 9
                        tmpInt=tmpInt>>2; // move bits 8 and 9 to 6 and 7 position
                        tmpInt=tmpInt|0xE; // insert opcode 0xE
                        fprintf(arqOut,"%X\n",tmpInt);
                        tmpInt=tmpAddr&0xFF; // extract bits 0 to 7
                        fprintf(arqOut,"%X\n",tmpInt); // addr
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // wr addr, b
                    if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                    {
                        // wr addr, b
                        tmpInt=tmpAddr&0x300; // extract bits 8 and 9
                        tmpInt=tmpInt>>2; // move bits 8 and 9 to 6 and 7 position
                        tmpInt=tmpInt|0xF; // insert opcode 0xF
                        fprintf(arqOut,"%X\n",tmpInt);
                        tmpInt=tmpAddr&0xFF; // extract bits 0 to 7
                        fprintf(arqOut,"%X\n",tmpInt); // addr
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
            }
            //RD
            if(strcmp(list->token,"rd")==0 && instructionFiniched==0)
            {
                list=list->next;
                // rd a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // rd a, ar
                    if(strcmp(list->token,"ar")==0)
                    {
                        fprintf(arqOut,"10\n"); // rd a, ar
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // rd a, ar+
                    if(strcmp(list->token,"ar+")==0 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"12\n"); // rd a, ar+
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // rd a, addr
                    if((list->type==4||list->type==7) && instructionFiniched==0)
                    {
                        if(list->type==4) // is a number
                        {
                            tmpAddr=extractNumber(list->token);
                        }
                        else // type 7
                        {
                            tmpAddr=list->addr;
                        }
                        tmpInt=tmpAddr&0x300; // extract bits 8 and 9
                        tmpInt=tmpInt>>2; // move bits 8 and 9 to 6 and 7 position
                        tmpInt=tmpInt|0x14; // insert opcode 0x14
                        fprintf(arqOut,"%X\n",tmpInt);
                        tmpInt=tmpAddr&0xFF; // extract bits 0 to 7
                        fprintf(arqOut,"%X\n",tmpInt); // addr
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
                // rd b,
                if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // rd b, ar
                    if(strcmp(list->token,"ar")==0 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"11\n"); // rd b, ar
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // rd b, ar+
                    if(strcmp(list->token,"ar+")==0 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"13\n"); // rd b, ar+
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // rd b, addr
                    if((list->type==4||list->type==7) && instructionFiniched==0)
                    {
                        if(list->type==4) // is a number
                        {
                            tmpAddr=extractNumber(list->token);
                        }
                        else // type 7
                        {
                            tmpAddr=list->addr;
                        }
                        tmpInt=tmpAddr&0x300; // extract bits 8 and 9
                        tmpInt=tmpInt>>2; // move bits 8 and 9 to 6 and 7 position
                        tmpInt=tmpInt|0x15; // insert opcode 0x15
                        fprintf(arqOut,"%X\n",tmpInt);
                        tmpInt=tmpAddr&0xFF; // extract bits 0 to 7
                        fprintf(arqOut,"%X\n",tmpInt); // addr
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
            }
            //IN
            if(strcmp(list->token,"in")==0 && instructionFiniched==0)
            {
                list=list->next;
                // in a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // in a, ar
                    if(strcmp(list->token,"ar")==0 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"16\n"); // in a, ar
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // in a, ar+
                    if(strcmp(list->token,"ar+")==0 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"18\n"); // in a, ar+
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // in a, addr
                    if((list->type==4||list->type==7) && instructionFiniched==0)
                    {
                        if(list->type==4) // is a number
                        {
                            tmpAddr=extractNumber(list->token);
                        }
                        else // type 7
                        {
                            tmpAddr=list->addr;
                        }
                        tmpInt=tmpAddr&0x300; // extract bits 8 and 9
                        tmpInt=tmpInt>>2; // move bits 8 and 9 to 6 and 7 position
                        tmpInt=tmpInt|0x1A; // insert opcode 0x1A
                        fprintf(arqOut,"%X\n",tmpInt);
                        tmpInt=tmpAddr&0xFF; // extract bits 0 to 7
                        fprintf(arqOut,"%X\n",tmpInt); // addr
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
                // in b,
                if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // in b, ar
                    if(strcmp(list->token,"ar")==0 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"17\n"); // in b, ar
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // in b, ar+
                    if(strcmp(list->token,"ar+")==0 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"19\n"); // in b, ar+
                        invalidInstruction=0;
                    }
                    // in b, addr
                    if((list->type==4||list->type==7) && instructionFiniched==0)
                    {
                        if(list->type==4) // is a number
                        {
                            tmpAddr=extractNumber(list->token);
                        }
                        else // type 7
                        {
                            tmpAddr=list->addr;
                        }
                        tmpInt=tmpAddr&0x300; // extract bits 8 and 9
                        tmpInt=tmpInt>>2; // move bits 8 and 9 to 6 and 7 position
                        tmpInt=tmpInt|0x1B; // insert opcode 0x1B
                        fprintf(arqOut,"%X\n",tmpInt);
                        tmpInt=tmpAddr&0xFF; // extract bits 0 to 7
                        fprintf(arqOut,"%X\n",tmpInt); // addr
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
            }
            //OUT
            if(strcmp(list->token,"out")==0 && instructionFiniched==0)
            {
                list=list->next;
                // out ar,
                if(strcmp(list->token,"ar")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // out ar, a
                    if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"1C\n"); // out ar, a
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // out ar, b
                    if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"1D\n"); // out ar, b
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // out ar, val
                    if(list->type==4 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"1E\n"); // out ar, val
                        fprintf(arqOut,"%X\n",extractNumber(list->token)); // val
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
                // out ar+,
                if(strcmp(list->token,"ar+")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // out ar+, a
                    if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"1F\n"); // out ar+, a
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // out ar+, b
                    if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"20\n"); // out ar+, b
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // out ar+, val
                    if(list->type==4 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"21\n"); // out ar+, val
                        fprintf(arqOut,"%X\n",extractNumber(list->token)); // val
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
                // out addr,
                if((list->type==4||list->type==7) && instructionFiniched==0)
                {
                    if(list->type==4) // is a number
                    {
                        tmpAddr=extractNumber(list->token);
                    }
                    else // type 7
                    {
                        tmpAddr=list->addr;
                    }
                    list=list->next;
                    // out addr, a
                    if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                    {
                        // out addr, a
                        tmpInt=tmpAddr&0x300; // extract bits 8 and 9
                        tmpInt=tmpInt>>2; // move bits 8 and 9 to 6 and 7 position
                        tmpInt=tmpInt|0x22; // insert opcode 0x22
                        fprintf(arqOut,"%X\n",tmpInt);
                        tmpInt=tmpAddr&0xFF; // extract bits 0 to 7
                        fprintf(arqOut,"%X\n",tmpInt); // addr
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // out addr, b
                    if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                    {
                        // out addr, b
                        tmpInt=tmpAddr&0x300; // extract bits 8 and 9
                        tmpInt=tmpInt>>2; // move bits 8 and 9 to 6 and 7 position
                        tmpInt=tmpInt|0x23; // insert opcode 0x23
                        fprintf(arqOut,"%X\n",tmpInt);
                        tmpInt=tmpAddr&0xFF; // extract bits 0 to 7
                        fprintf(arqOut,"%X\n",tmpInt); // addr
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
            }
            //INC
            if(strcmp(list->token,"inc")==0 && instructionFiniched==0)
            {
                list=list->next;
                // inc a
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    fprintf(arqOut,"24\n"); // inc a
                    invalidInstruction=0;
                    instructionFiniched=1;
                }
                // inc b
                if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                {
                    fprintf(arqOut,"25\n"); // inc b
                    invalidInstruction=0;
                    instructionFiniched=1;
                }
                // inc ar
                if(strcmp(list->token,"ar")==0 && instructionFiniched==0)
                {
                    fprintf(arqOut,"26\n"); // inc ar
                    invalidInstruction=0;
                    instructionFiniched=1;
                }
            }
            //JMP
             if(strcmp(list->token,"jmp")==0 && instructionFiniched==0)
            {
                list=list->next;
                // jmp addr
                if((list->type==4||list->type==1) && instructionFiniched==0)
                {
                    if(list->type==4) // is a number
                    {
                        tmpAddr=extractNumber(list->token);
                    }
                    else // type 1
                    {
                        tmpAddr=list->addr;
                    }
                    tmpInt=tmpAddr&0x300; // extract bits 8 and 9
                    tmpInt=tmpInt>>2; // move bits 8 and 9 to 6 and 7 position
                    tmpInt=tmpInt|0x27; // insert opcode 0x27
                    fprintf(arqOut,"%X\n",tmpInt);
                    tmpInt=tmpAddr&0xFF; // extract bits 0 to 7
                    fprintf(arqOut,"%X\n",tmpInt); // addr
                    addr++;
                    invalidInstruction=0;
                    instructionFiniched=1;
                }
            }
            //JIZ
            if(strcmp(list->token,"jiz")==0 && instructionFiniched==0)
            {
                list=list->next;
                // jiz a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // jiz a, addr
                    if((list->type==4||list->type==1) && instructionFiniched==0)
                    {
                        if(list->type==4) // is a number
                        {
                            tmpAddr=extractNumber(list->token);
                        }
                        else // type 1
                        {
                            tmpAddr=list->addr;
                        }
                        tmpInt=tmpAddr&0x300; // extract bits 8 and 9
                        tmpInt=tmpInt>>2; // move bits 8 and 9 to 6 and 7 position
                        tmpInt=tmpInt|0x28; // insert opcode 0x28
                        fprintf(arqOut,"%X\n",tmpInt);
                        tmpInt=tmpAddr&0xFF; // extract bits 0 to 7
                        fprintf(arqOut,"%X\n",tmpInt); // addr
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
            }
            //JIE
            if(strcmp(list->token,"jie")==0 && instructionFiniched==0)
            {
                list=list->next;
                // jie a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // jie a, b,
                    if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                    {
                        list=list->next;
                        // jie a, b, addr
                        if(list->type==4||list->type==1)
                        {
                            if(list->type==4) // is a number
                            {
                                tmpAddr=extractNumber(list->token);
                            }
                            else // type 1
                            {
                                tmpAddr=list->addr;
                            }
                            tmpInt=tmpAddr&0x300; // extract bits 8 and 9
                            tmpInt=tmpInt>>2; // move bits 8 and 9 to 6 and 7 position
                            tmpInt=tmpInt|0x29; // insert opcode 0x29
                            fprintf(arqOut,"%X\n",tmpInt);
                            tmpInt=tmpAddr&0xFF; // extract bits 0 to 7
                            fprintf(arqOut,"%X\n",tmpInt); // addr
                            addr++;
                            invalidInstruction=0;
                            instructionFiniched=1;
                        }
                    }
                }
            }
            //JIG
            if(strcmp(list->token,"jig")==0 && instructionFiniched==0)
            {
                list=list->next;
                // jig a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // jig a, b,
                    if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                    {
                        list=list->next;
                        // jig a, b, addr
                        if(list->type==4||list->type==1)
                        {
                            if(list->type==4) // is a number
                            {
                                tmpAddr=extractNumber(list->token);
                            }
                            else // type 1
                            {
                                tmpAddr=list->addr;
                            }
                            tmpInt=tmpAddr&0x300; // extract bits 8 and 9
                            tmpInt=tmpInt>>2; // move bits 8 and 9 to 6 and 7 position
                            tmpInt=tmpInt|0x2A; // insert opcode 0x2A
                            fprintf(arqOut,"%X\n",tmpInt);
                            tmpInt=tmpAddr&0xFF; // extract bits 0 to 7
                            fprintf(arqOut,"%X\n",tmpInt); // addr
                            addr++;
                            invalidInstruction=0;
                            instructionFiniched=1;
                        }
                    }
                }
            }
            //JIS
            if(strcmp(list->token,"jis")==0 && instructionFiniched==0)
            {
                list=list->next;
                // jis a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // jis a, b,
                    if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                    {
                        list=list->next;
                        // jis a, b, addr
                        if(list->type==4||list->type==1)
                        {
                            if(list->type==4) // is a number
                            {
                                tmpAddr=extractNumber(list->token);
                            }
                            else // type 1
                            {
                                tmpAddr=list->addr;
                            }
                            tmpInt=tmpAddr&0x300; // extract bits 8 and 9
                            tmpInt=tmpInt>>2; // move bits 8 and 9 to 6 and 7 position
                            tmpInt=tmpInt|0x2B; // insert opcode 0x2A
                            fprintf(arqOut,"%X\n",tmpInt);
                            tmpInt=tmpAddr&0xFF; // extract bits 0 to 7
                            fprintf(arqOut,"%X\n",tmpInt); // addr
                            addr++;
                            invalidInstruction=0;
                            instructionFiniched=1;
                        }
                    }
                }
            }
            //JIN
            if(strcmp(list->token,"jin")==0 && instructionFiniched==0)
            {
                list=list->next;
                // jin a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // jin a, addr
                    if((list->type==4||list->type==1) && instructionFiniched==0)
                    {
                        if(list->type==4) // is a number
                        {
                            tmpAddr=extractNumber(list->token);
                        }
                        else // type 1
                        {
                            tmpAddr=list->addr;
                        }
                        tmpInt=tmpAddr&0x300; // extract bits 8 and 9
                        tmpInt=tmpInt>>2; // move bits 8 and 9 to 6 and 7 position
                        tmpInt=tmpInt|0x2C; // insert opcode 0x2C
                        fprintf(arqOut,"%X\n",tmpInt);
                        tmpInt=tmpAddr&0xFF; // extract bits 0 to 7
                        fprintf(arqOut,"%X\n",tmpInt); // addr
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
            }
            //JIC
            if(strcmp(list->token,"jic")==0 && instructionFiniched==0)
            {
                list=list->next;
                // jic a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // jic a, addr
                    if(list->type==4||list->type==1)
                    {
                        if(list->type==4) // is a number
                        {
                            tmpAddr=extractNumber(list->token);
                        }
                        else // type 1
                        {
                            tmpAddr=list->addr;
                        }
                        tmpInt=tmpAddr&0x300; // extract bits 8 and 9
                        tmpInt=tmpInt>>2; // move bits 8 and 9 to 6 and 7 position
                        tmpInt=tmpInt|0x2D; // insert opcode 0x2D
                        fprintf(arqOut,"%X\n",tmpInt);
                        tmpInt=tmpAddr&0xFF; // extract bits 0 to 7
                        fprintf(arqOut,"%X\n",tmpInt); // addr
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
            }
            //SET
            if(strcmp(list->token,"set")==0 && instructionFiniched==0)
            {
                list=list->next;
                // set c
                if(strcmp(list->token,"c")==0 && instructionFiniched==0) // set c
                {
                    fprintf(arqOut,"2E\n");
                    invalidInstruction=0;
                    instructionFiniched=1;
                }
            }
            //CLR
            if(strcmp(list->token,"clr")==0 && instructionFiniched==0)
            {
                list=list->next;
                // clr c
                if(strcmp(list->token,"c")==0 && instructionFiniched==0)
                {
                    fprintf(arqOut,"2F\n"); // not a
                    invalidInstruction=0;
                    instructionFiniched=1;
                }
            }
            //NOT
            if(strcmp(list->token,"not")==0 && instructionFiniched==0)
            {
                list=list->next;
                // not a
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    fprintf(arqOut,"30\n"); // not a
                    invalidInstruction=0;
                    instructionFiniched=1;
                }
            }
            //OR
            if(strcmp(list->token,"or")==0 && instructionFiniched==0)
            {
                list=list->next;
                // or a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // or a, b
                    if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"31\n"); // or a, b
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // or a, val
                    if(list->type==4 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"32\n"); // or a, val
                        fprintf(arqOut,"%X\n",extractNumber(list->token)); // val
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
            }
            //AND
            if(strcmp(list->token,"and")==0 && instructionFiniched==0)
            {
                list=list->next;
                // and a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // and a, b
                    if(strcmp(list->token,"b")==0)
                    {
                        fprintf(arqOut,"33\n"); // and a, b
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // and a, val
                    if(list->type==4 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"34\n"); // and a, val
                        fprintf(arqOut,"%X\n",extractNumber(list->token)); // val
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
            }
            //XOR
            if(strcmp(list->token,"xor")==0 && instructionFiniched==0)
            {
                list=list->next;
                // xor a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // xor a, b
                    if(strcmp(list->token,"b")==0)
                    {
                        fprintf(arqOut,"35\n"); // xor a, b
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // xor a, val
                    if(list->type==4 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"36\n"); // xor a, val
                        fprintf(arqOut,"%X\n",extractNumber(list->token)); // val
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
            }
            //ADD
            if(strcmp(list->token,"add")==0 && instructionFiniched==0)
            {
                list=list->next;
                // add a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // add a, b
                    if(strcmp(list->token,"b")==0)
                    {
                        fprintf(arqOut,"37\n"); // add a, b
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // add a, val
                    if(list->type==4 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"38\n"); // add a, val
                        fprintf(arqOut,"%X\n",extractNumber(list->token)); // val
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
            }
            //SUB
            if(strcmp(list->token,"sub")==0 && instructionFiniched==0)
            {
                list=list->next;
                // sub a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // sub a, b
                    if(strcmp(list->token,"b")==0)
                    {
                        fprintf(arqOut,"39\n"); // sub a, b
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // sub a, val
                    if(list->type==4 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"3A\n"); // sub a, val
                        fprintf(arqOut,"%X\n",extractNumber(list->token)); // val
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
            }
            //DEC
            if(strcmp(list->token,"dec")==0 && instructionFiniched==0)
            {
                list=list->next;
                // dec a
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    fprintf(arqOut,"3B\n"); // dec a
                    invalidInstruction=0;
                    instructionFiniched=1;
                }
                // dec b
                if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                {
                    fprintf(arqOut,"3C\n"); // dec b
                    invalidInstruction=0;
                    instructionFiniched=1;
                }
            }
            //LSL
            if(strcmp(list->token,"lsl")==0 && instructionFiniched==0)
            {
                list=list->next;
                // lsl a
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    fprintf(arqOut,"3D\n"); // lsl a
                    invalidInstruction=0;
                    instructionFiniched=1;
                }
            }
            //LSR
            if(strcmp(list->token,"lsr")==0 && instructionFiniched==0)
            {
                list=list->next;
                // lsr a
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    fprintf(arqOut,"3E\n"); // lsr a
                    invalidInstruction=0;
                    instructionFiniched=1;
                }
            }
            //SWAP
            if(strcmp(list->token,"swap")==0 && instructionFiniched==0)
            {
                list=list->next;
                // swap a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // swap a, b
                    if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                    {
                        fprintf(arqOut,"3F\n"); // sap a, b
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
            }
            if(invalidInstruction==1)
            {
                printf("[ERROR Line %d] Invalid instruction near to (%s)\n",list->lineNumber,list->token);
                ret=1;
            }
        }
        list=list->next;
    }
    printf("PROM = %d bytes",addr);
    return(ret);
}

*/

int compile32i(struct tokenList* list) // Hneemaan Digital format
{
    int ret=0; // return value, 0=no error
    //int tmpInt;
    int tmpAddr;
    int invalidInstruction;
    int instructionFiniched;

    while(list!=NULL)
    {
        if(list->type==12) //PROM constant Data
        {
            list=list->next;
            PROM[addr]=extractNumber(list->token); // add data to PROM
            addr++;
        }

        if(list->type==2||list->type==5) //instruction
        {
            //printf("inst -> %s, %s (l%d)\n",list->token,list->next->token,list->lineNumber);
            invalidInstruction=1;
            instructionFiniched=0;
            // NOP
            if(strcmp(list->token,"nop")==0)
            {
                PROM[addr]=0; // nop
                addr++;
                invalidInstruction=0;
                instructionFiniched=1;
            }
            //LD
            if(strcmp(list->token,"ld")==0 && instructionFiniched==0)
            {
                list=list->next;
                // ld a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // ld a, val
                    if(list->type==4 && instructionFiniched==0)
                    {
                        PROM[addr]=1; // ld a,
                        addr++;
                        PROM[addr]=extractNumber(list->token); // val
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // ld a, b
                    if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                    {
                        PROM[addr]=3; // ld a,b
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    else
                    {
                        instructionFiniched=1;
                    }
                }
                // ld b,
                if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // ld b, val
                    if(list->type==4||list->type==7)
                    {
                        if(list->type==4) // is a number
                        {
                            tmpAddr=extractNumber(list->token);
                        }
                        else // type 7
                        {
                            tmpAddr=list->addr;
                        }
                        PROM[addr]=2; // ld b,
                        addr++;
                        PROM[addr]=tmpAddr;
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    else
                    {
                        instructionFiniched=1;
                    }
                }
            }
            //MOV
            if(strcmp(list->token,"mov")==0 && instructionFiniched==0)
            {
                list=list->next;
                // mov b,
                if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // mov b, a
                    if(strcmp(list->token,"a")==0)
                    {
                        PROM[addr]=4; // mov b, a
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
                // mov a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // mov a, b
                    if(strcmp(list->token,"b")==0)
                    {
                        PROM[addr]=5; // mov a, b
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
                else
                {
                    instructionFiniched=1;
                }
            }
            //WR
            if(strcmp(list->token,"wr")==0 && instructionFiniched==0)
            {
                list=list->next;
                // wr b,
                if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // wr b, a
                    if(strcmp(list->token,"a")==0)
                    {
                        PROM[addr]=6; // wr b, a
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
                // wr addr,
                if((list->type==4||list->type==7) && instructionFiniched==0)
                {
                    if(list->type==4) // is a number
                    {
                        tmpAddr=extractNumber(list->token);
                    }
                    else // type 7
                    {
                        tmpAddr=list->addr;
                    }
                    list=list->next;
                    // wr addr, a
                    if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                    {
                        PROM[addr]=7; // wr addr, a
                        addr++;
                        PROM[addr]=tmpAddr;
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // wr addr, b
                    if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                    {
                        PROM[addr]=8; // wr addr, b
                        addr++;
                        PROM[addr]=tmpAddr;
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    else
                    {
                        instructionFiniched=1;
                    }
                }
            }
            //RD
            if(strcmp(list->token,"rd")==0 && instructionFiniched==0)
            {
                list=list->next;
                // rd a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // rd a, b
                    if(strcmp(list->token,"b")==0)
                    {
                        PROM[addr]=9; // rd a, b
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    // rd a, addr
                    if((list->type==4||list->type==7) && instructionFiniched==0)
                    {
                        if(list->type==4) // is a number
                        {
                            tmpAddr=extractNumber(list->token);
                        }
                        else // type 7
                        {
                            tmpAddr=list->addr;
                        }
                        PROM[addr]=0xA; // rd a, addr
                        addr++;
                        PROM[addr]=tmpAddr;
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
                // rd b,
                if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // rd b, addr
                    if((list->type==4||list->type==7) && instructionFiniched==0)
                    {
                        if(list->type==4) // is a number
                        {
                            tmpAddr=extractNumber(list->token);
                        }
                        else // type 7
                        {
                            tmpAddr=list->addr;
                        }
                        PROM[addr]=0xB; // rd b, addr
                        addr++;
                        PROM[addr]=tmpAddr;
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                }
                else
                {
                    instructionFiniched=1;
                }
            }
            //IN
            if(strcmp(list->token,"in")==0 && instructionFiniched==0)
            {
                list=list->next;
                // in a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // in a, addr
                    if((list->type==4||list->type==7) && instructionFiniched==0)
                    {
                        if(list->type==4) // is a number
                        {
                            tmpAddr=extractNumber(list->token);
                        }
                        else // type 7
                        {
                            tmpAddr=list->addr;
                        }
                        PROM[addr]=0xC; // in a, addr
                        addr++;
                        PROM[addr]=tmpAddr;
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    else
                    {
                        instructionFiniched=1;
                    }
                }
            }
            //OUT
            if(strcmp(list->token,"out")==0 && instructionFiniched==0)
            {
                list=list->next;
                // out addr,
                if((list->type==4||list->type==7) && instructionFiniched==0)
                {
                    if(list->type==4) // is a number
                    {
                        tmpAddr=extractNumber(list->token);
                    }
                    else // type 7
                    {
                        tmpAddr=list->addr;
                    }
                    list=list->next;
                    // out addr, a
                    if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                    {
                        PROM[addr]=0xD; // out addr, a
                        addr++;
                        PROM[addr]=tmpAddr;
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    else
                    {
                        instructionFiniched=1;
                    }
                }
            }
            //JMP
            if(strcmp(list->token,"jmp")==0 && instructionFiniched==0)
            {
                list=list->next;
                // jmp b
                if(strcmp(list->token,"b")==0)
                {
                    PROM[addr]=0xE; // jmp b
                    addr++;
                    invalidInstruction=0;
                    instructionFiniched=1;
                }
                // jmp addr
                if((list->type==4||list->type==1) && instructionFiniched==0)
                {
                    if(list->type==4) // is a number
                    {
                        tmpAddr=extractNumber(list->token);
                    }
                    else // type 1
                    {
                        tmpAddr=list->addr;
                    }
                    PROM[addr]=0xF; // jmp addr
                    addr++;
                    PROM[addr]=tmpAddr;
                    addr++;
                    invalidInstruction=0;
                    instructionFiniched=1;
                }
                else
                {
                    instructionFiniched=1;
                }
            }
            //JIZ
            if(strcmp(list->token,"jiz")==0 && instructionFiniched==0)
            {
                list=list->next;
                // jiz a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // jiz a, addr
                    if((list->type==4||list->type==1) && instructionFiniched==0)
                    {
                        if(list->type==4) // is a number
                        {
                            tmpAddr=extractNumber(list->token);
                        }
                        else // type 1
                        {
                            tmpAddr=list->addr;
                        }
                        PROM[addr]=0x10; // jiz a, addr
                        addr++;
                        PROM[addr]=tmpAddr;
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    else
                    {
                        instructionFiniched=1;
                    }
                }
            }
            //JIE
            if(strcmp(list->token,"jie")==0 && instructionFiniched==0)
            {
                list=list->next;
                // jie a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // jie a, b,
                    if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                    {
                        list=list->next;
                        // jie a, b, addr
                        if(list->type==4||list->type==1)
                        {
                            if(list->type==4) // is a number
                            {
                                tmpAddr=extractNumber(list->token);
                            }
                            else // type 1
                            {
                                tmpAddr=list->addr;
                            }
                            PROM[addr]=0x11; // jie a, b, addr
                            addr++;
                            PROM[addr]=tmpAddr;
                            addr++;
                            invalidInstruction=0;
                            instructionFiniched=1;
                        }
                        else
                        {
                            instructionFiniched=1;
                        }
                    }
                }
            }
            //JIG
            if(strcmp(list->token,"jig")==0 && instructionFiniched==0)
            {
                list=list->next;
                // jig a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // jig a, b,
                    if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                    {
                        list=list->next;
                        // jig a, b, addr
                        if(list->type==4||list->type==1)
                        {
                            if(list->type==4) // is a number
                            {
                                tmpAddr=extractNumber(list->token);
                            }
                            else // type 1
                            {
                                tmpAddr=list->addr;
                            }
                            PROM[addr]=0x12; // jig a, b, addr
                            addr++;
                            PROM[addr]=tmpAddr;
                            addr++;
                            invalidInstruction=0;
                            instructionFiniched=1;
                        }
                        else
                        {
                            instructionFiniched=1;
                        }
                    }
                }
            }
            //JIS
            if(strcmp(list->token,"jis")==0 && instructionFiniched==0)
            {
                list=list->next;
                // jis a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // jis a, b,
                    if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                    {
                        list=list->next;
                        // jis a, b, addr
                        if(list->type==4||list->type==1)
                        {
                            if(list->type==4) // is a number
                            {
                                tmpAddr=extractNumber(list->token);
                            }
                            else // type 1
                            {
                                tmpAddr=list->addr;
                            }
                            PROM[addr]=0x13; // jis a, b, addr
                            addr++;
                            PROM[addr]=tmpAddr;
                            addr++;
                            invalidInstruction=0;
                            instructionFiniched=1;
                        }
                        else
                        {
                            instructionFiniched=1;
                        }
                    }
                }
            }
            //JIN
            if(strcmp(list->token,"jin")==0 && instructionFiniched==0)
            {
                list=list->next;
                // jin a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // jin a, addr
                    if((list->type==4||list->type==1) && instructionFiniched==0)
                    {
                        if(list->type==4) // is a number
                        {
                            tmpAddr=extractNumber(list->token);
                        }
                        else // type 1
                        {
                            tmpAddr=list->addr;
                        }
                        PROM[addr]=0x14; // jin a, addr
                        addr++;
                        PROM[addr]=tmpAddr;
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    else
                    {
                        instructionFiniched=1;
                    }
                }
            }
            //JIC
            if(strcmp(list->token,"jic")==0 && instructionFiniched==0)
            {
                list=list->next;
                // jic a, addr
                if(list->type==4||list->type==1)
                {
                    if(list->type==4) // is a number
                    {
                        tmpAddr=extractNumber(list->token);
                    }
                    else // type 1
                    {
                        tmpAddr=list->addr;
                    }
                    PROM[addr]=0x15; // jic addr
                    addr++;
                    PROM[addr]=tmpAddr;
                    addr++;
                    invalidInstruction=0;
                    instructionFiniched=1;
                }
                else
                {
                    instructionFiniched=1;
                }
            }
            //SET
            if(strcmp(list->token,"set")==0 && instructionFiniched==0)
            {
                list=list->next;
                // set c
                if(strcmp(list->token,"c")==0 && instructionFiniched==0) // set c
                {
                    PROM[addr]=0x16; // set c
                    addr++;
                    invalidInstruction=0;
                    instructionFiniched=1;
                }
                else
                {
                    instructionFiniched=1;
                }
            }
            //CLR
            if(strcmp(list->token,"clr")==0 && instructionFiniched==0)
            {
                list=list->next;
                // clr c
                if(strcmp(list->token,"c")==0 && instructionFiniched==0)
                {
                    PROM[addr]=0x17; // clr c
                    addr++;
                    invalidInstruction=0;
                    instructionFiniched=1;
                }
                else
                {
                    instructionFiniched=1;
                }
            }
            //NOT
            if(strcmp(list->token,"not")==0 && instructionFiniched==0)
            {
                list=list->next;
                // not a
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    PROM[addr]=0x18; // nor a
                    addr++;
                    invalidInstruction=0;
                    instructionFiniched=1;
                }
                else
                {
                    instructionFiniched=1;
                }
            }
            //OR
            if(strcmp(list->token,"or")==0 && instructionFiniched==0)
            {
                list=list->next;
                // or a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // or a, b
                    if(strcmp(list->token,"b")==0 && instructionFiniched==0)
                    {
                        PROM[addr]=0x19; // or a, b
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    else
                    {
                        instructionFiniched=1;
                    }
                }
            }
            //AND
            if(strcmp(list->token,"and")==0 && instructionFiniched==0)
            {
                list=list->next;
                // and a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // and a, b
                    if(strcmp(list->token,"b")==0)
                    {
                        PROM[addr]=0x1A; // and a, b
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    else
                    {
                        instructionFiniched=1;
                    }
                }
            }
            //XOR
            if(strcmp(list->token,"xor")==0 && instructionFiniched==0)
            {
                list=list->next;
                // xor a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // xor a, b
                    if(strcmp(list->token,"b")==0)
                    {
                        PROM[addr]=0x1B; // xor a, b
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    else
                    {
                        instructionFiniched=1;
                    }
                }
            }
            //ADD
            if(strcmp(list->token,"add")==0 && instructionFiniched==0)
            {
                list=list->next;
                // add a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // add a, b
                    if(strcmp(list->token,"b")==0)
                    {
                        PROM[addr]=0x1C; // add a, b
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    else
                    {
                        instructionFiniched=1;
                    }
                }
            }
            //SUB
            if(strcmp(list->token,"sub")==0 && instructionFiniched==0)
            {
                list=list->next;
                // sub a,
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    list=list->next;
                    // sub a, b
                    if(strcmp(list->token,"b")==0)
                    {
                        PROM[addr]=0x1D; // sub a, b
                        addr++;
                        invalidInstruction=0;
                        instructionFiniched=1;
                    }
                    else
                    {
                        instructionFiniched=1;
                    }
                }
            }
            //LSL
            if(strcmp(list->token,"lsl")==0 && instructionFiniched==0)
            {
                list=list->next;
                // lsl a
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    PROM[addr]=0x1E; // lsl a
                    addr++;
                    invalidInstruction=0;
                    instructionFiniched=1;
                }
                else
                {
                    instructionFiniched=1;
                }
            }
            //LSR
            if(strcmp(list->token,"lsr")==0 && instructionFiniched==0)
            {
                list=list->next;
                // lsr a
                if(strcmp(list->token,"a")==0 && instructionFiniched==0)
                {
                    PROM[addr]=0x1F; // lsr a
                    addr++;
                    invalidInstruction=0;
                    instructionFiniched=1;
                }
                else
                {
                    instructionFiniched=1;
                }
            }
            if(invalidInstruction==1)
            {
                printf("[ERROR Line %d] Invalid instruction near to (%s)\n",list->lineNumber,list->token);
                ret=1;
            }
        }
        list=list->next;
    }
    printf("PROM = %d bytes",addr);
    return(ret);
}

void saveOutFileHex(FILE* arqOut)
{
    fprintf(arqOut,"v2.0 raw\n");
    for(int i=0; i<addr; i++)
    {
        fprintf(arqOut,"%X\n",PROM[i]); // val
    }
}

int main(int argc, char *argv[])
{
    int error=0;
    FILE* arqIn;
    FILE* arqOut;
    struct tokenList* tokens = NULL;

    printf("\nAssembler V0.1\n");
    printf("Assembling: \"%s\" to \"%s\"\n",argv[1],argv[2]);


    if(argc<3)
    {
        printf("\nMissing arguments: use AssemblerCpuD.exe \"inputFile.ext\" \"outputFile.ext\"\n");
        return(0);
    }

    arqIn = fopen(argv[1], "r");

    if(arqIn == NULL) // testa se o arquivo foi aberto com sucesso
    {
        printf("\n\nUnable to open input file!\n\n");
        return 0;
    }

    arqOut = fopen(argv[2], "w");

    if(arqOut == NULL) // testa se o arquivo foi aberto com sucesso
    {
        printf("\n\nUnable to open output file!\n\n");
        return 0;
    }

    error+=extractTokens(&tokens, arqIn);
    error+=analizeTokens(&tokens);
    error+=extractDefines(tokens);
    error+=setDefines(tokens);
    error+=extractDataAddr(tokens);
    error+=setDataAddr(tokens);
    error+=extract2BytsInstruction(tokens);
    error+=extractLabels(tokens);
    error+=setLabels(tokens);
    error+=extractConst(tokens);
    error+=updateConst(tokens);
    error+=setConst(tokens);
    print(tokens); // for tests only
    error+=errorCheck(tokens);
    error+=compile32i(tokens);
    if(error==0)
    {
        saveOutFileHex(arqOut);
        printf("\nCompiled successfully!\n\n");
    }
    else
    {
        printf("\nErrors were found during compilation!\n\n");
    }
    fclose(arqIn);
    fclose(arqOut);
    return 0;
}
