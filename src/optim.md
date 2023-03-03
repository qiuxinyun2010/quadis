### 水塘抽样
```C++
/*Before*/
listlen = 0;
orighe = he;
while(he) {
    he = he->next;
    listlen++;
}
listele = random() % listlen;
he = orighe;
while(listele--) he = he->next;
return he;

/*After*/
listlen = 0;
while(ps_he!=0) {
    he = (dictEntry *)void_ptr(ps_he);
    listlen++;
    if((random() % listlen)==0) 
    ret = he;
    ps_he = he->next;
}
return ret;
```