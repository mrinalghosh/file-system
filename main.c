#include <stdio.h>
#include <string.h>

#include "disk.h"
#include "fs.h"

int main(int argc, char** argv) {
    char* disk = "C";

    if (make_fs(disk) == -1) {
        printf("make_fs error\n");
        return -1;
    }

    if (mount_fs(disk) == -1) {
        printf("mount_fs error\n");
        return -1;
    }

    char* files[2];
    files[0] = "doom.exe";
    files[1] = "minecraft.exe";

    /* CREATE TEST */
    fs_create(files[0]);
    fs_create(files[1]);

    /* OPEN TEST */
    int fd[MAX_FILDES];

    // fd[0] = fs_open(files[0]); // DOOM
    // fd[2] = fs_open(files[0]); // DOOM
    // fd[1] = fs_open(files[1]); // MINECRAFT

    /* MULTIPLE OPEN TEST */
    fd[0] = fs_open(files[0]);  // DOOM
    fd[1] = fs_open(files[0]);  // DOOM

    /* WRITE TEST */
    char* data = "aizjgbfhzwdkdmgasscnpjhkyhrsidhtykjdimhojnentamyhpwlhnzwgnbkcnmotznqnvfgalzbrklddlbyvbqahsbeikpfoonbwgwcnaawbhrzgvmzyvapzcdqmxmzhhjfaewdqwgxocrxowuhmhyrqkkhonkraodnayqdopapirbduzzhotexndktksmdnpjhmteghucubecpelqfljdvrspdxbjwdsrcwcmtyxshowektrhqzzbovhthlkvdgwrjemeihbiyrqznpkcfdacbnnkqqfxeehqnqcimoszrhmefyqnuppmyytaugirjsztoaqgmzqywhdgwhcaguvgopphnuvuccamfpcdshufemrfremjzbgslrevfjkprymjmfbdmzfntlfoxxhgkttlhtstnwsmloljpkvpyeggtowsoidlgsphfpaqpbqfrbmkyiaydgitorokwpcbupcynejsvaafmqlgnkvpewkaxdakrsdwwanycjorxbvvvrlghzmcvvqtfzbccwciahnfgslxjplstjxmxehsshavkzbdctcbrdqgmsfovxuqvntitvckajznlfsjviwekiawuqlscujvkucbxabzzelpfhadpyoibhaztzvampyuigherajzhxylchxjjmspijsqltdkrywgppxqgysldopxajrtqajchvmvecanhaggcmjlzxzykfhclvqmblircxrodpsixiocgxejoxjsbkucsumyyeomwxxsbrzppdnqchbtfbwudmahcnwmuumcfinnyprylxzfdqgpokkbflrfnenemclmkdskteeywdqesflbslyrfwhevlcweiuiulylzgaimhmcfgshnahtinhxzaynojpzfnebyvfhcsegkqglmpatvevazqrxdyvbaoscmlzfqfvcdnsdpvtoyyoagsjxvshxkzlgqloevlgckzrldegljphfwgvjqensvckwsnztkefirgxbjbzvhizfkiybjeykdjcyirydafirkqdzrtwtninktbnqstffrtlcvcpgmefhoigtetjtbmrrypibdrnhbcpjxoctptzjezyupjxpkozmqzumodxersrsxeglmofcupsnsevpmhtwmlpfzuvnzymlwenoytazcuvpcrttgxwdtjlclgoczjjqubptwtvwvmtybefecdaewzguprvuieszvhrybovlikraljgcrjlzdehhiybhmxdlgndxaqzeyynnptqljxpkvacjkjwsxagkinxzcuxgxuranqmtnrddxfjoobdybizwqrnigirgsvmojqewwzbhjkmchlnngotlshnoxglsiumllvhxacarllpozmkntvctvkykwrurwfcsiyljgjhxstksalimmhkpofqrsizedktdzeaatexyukimofzttutcozhpvvuvgymseswrvxfeqckeimzmqmiqdeyhaxhcoupzlgxiehexymtpttivvblgmqtbkzmwkpxdtxnvdehzesnvhzlgiqtjgkfflbtkzmvobpfpwbaxlpmdmlxdrvtnigiigtvtqxwgrhnbaihowodaqshxgkkxbcdywwuilblsfvlbjgvyjkzgpmawpntmkmfqlgvukoxjwpdjqxjwgyuyutvayqcwgippluwjctwjbxlphqhdjotmljrbinyjyclbozdrouozszxdvrtmkuyzcwunfldwoqqtwxsmlueatuyvnpqfpbuirgkpgscbaevefnjumtbabwzxdrdynbyvzpvjrspzucjbraoaeevapmbjttictbqqhnlunxhgvdwqqvonuyekqqljocdwjxcnyyjidzcygpiobjkknndlsrmijtwdoaomwhbigybwglzxwqjteijlugieyfqtowzzaylqnftedvadoyaegwrcfbxbwcadecpnqzpaecwruqrldongaqykyecvnqxxogvfkfceeocmmgxicawpalnrnwohnindbfjdskgtenmmcowomttbexjdqecyhafdvqplscdncqpeomkjqcucs";

    // int nw = 2;  // number of blocks worth of buffers to write
    // char* wbuf = calloc(nw, BLOCK_SIZE);
    // int i;
    // for (i = 0; i < nw; ++i) {
    //     strcpy(wbuf + i * BLOCK_SIZE, data);
    // }

    // fs_write(fd[0], wbuf, nw * BLOCK_SIZE + 1);  // DOOM
    fs_write(fd[1], data, 4);  // DOOM
    fs_write(fd[1], data+4, 4); //DOOM

    // fs_write(fd[0], data, BLOCK_SIZE);

    /* SEEK TESTS */
    fs_lseek(fd[1], 2);

    /* READ TESTS */
    int nbyte = 4;
    char* rbuf = calloc(1, BLOCK_SIZE);
    fs_read(fd[1], rbuf, nbyte);  // DOOM
    printf("FS_READ %d bytes:\t%s\n", nbyte, rbuf);

    /* LIST TEST */
    // char*** filenames = calloc(1, sizeof(char**));
    // fs_listfiles(filenames);
    // printf("Files on disk %s:\n", disk);
    // char** temp = (*filenames);
    // while (*temp != NULL) {
    //     printf("\t%s\n", *temp);
    //     ++temp;
    // }

    /* FILESIZE TEST */
    // for (i = 0; i < 2; ++i)
    //     if ((*filenames)[i] != NULL)
    //         printf("Size of %s is %d\n", (*filenames)[i], fs_get_filesize(fd[i]));

    /* DELETE TEST */
    // fs_delete(files[0]);
    // fs_delete(files[1]);

    fs_close(fd[0]);
    fs_close(fd[1]);

    umount_fs(disk);

    return 0;
}