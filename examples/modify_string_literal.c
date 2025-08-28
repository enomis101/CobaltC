int main(){
        char *ptr = "abc";
        //undefined behavior -- seg fault
        ptr[0] = 'x';
        return 0;
}
