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

    int i;

    char* files[2];
    files[0] = "doom.exe";
    files[1] = "minecraft.exe";

    // char* files[MAX_FILDES] = {
    //     // only create 32 files and open them
    //     "doom.exe",
    //     "minecraft.exe",
    //     "limewire.exe",
    //     "cat.jpg",

    //     "dog.gif",
    //     "jurasicpark.mp4",
    //     "notes.docx",
    //     "resume.pages",

    //     "reptile.gpx",
    //     "lily&themoon.gpx",
    //     "coven.gpx",
    //     "diagram.fig",

    //     "friends.mp4",
    //     "vscode.exe",
    //     "slack.dmg",
    //     "tree.xml",

    //     "python",
    //     "ls.bin",
    //     "holiday.mp3",
    //     "buble.mp3",

    //     "entersandmn.mp3",
    //     "unforgiven.mp3",
    //     "humantarget.mp3",
    //     "fcpremix.mp3",

    //     "nightcore.mp3",
    //     "a.a",
    //     "b.b",
    //     "c.c",

    //     "d.d",
    //     "e.e",
    //     "f.f",
    //     "g.g",
    // };

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
    fd[1] = fs_open(files[1]);  // DOOM

    /* WRITE TEST */
    char* data = "aizjgbfhzwdkdmgasscnpjhkyhrsidhtykjdimhojnentamyhpwlhnzwgnbkcnmotznqnvfgalzbrklddlbyvbqahsbeikpfoonbwgwcnaawbhrzgvmzyvapzcdqmxmzhhjfaewdqwgxocrxowuhmhyrqkkhonkraodnayqdopapirbduzzhotexndktksmdnpjhmteghucubecpelqfljdvrspdxbjwdsrcwcmtyxshowektrhqzzbovhthlkvdgwrjemeihbiyrqznpkcfdacbnnkqqfxeehqnqcimoszrhmefyqnuppmyytaugirjsztoaqgmzqywhdgwhcaguvgopphnuvuccamfpcdshufemrfremjzbgslrevfjkprymjmfbdmzfntlfoxxhgkttlhtstnwsmloljpkvpyeggtowsoidlgsphfpaqpbqfrbmkyiaydgitorokwpcbupcynejsvaafmqlgnkvpewkaxdakrsdwwanycjorxbvvvrlghzmcvvqtfzbccwciahnfgslxjplstjxmxehsshavkzbdctcbrdqgmsfovxuqvntitvckajznlfsjviwekiawuqlscujvkucbxabzzelpfhadpyoibhaztzvampyuigherajzhxylchxjjmspijsqltdkrywgppxqgysldopxajrtqajchvmvecanhaggcmjlzxzykfhclvqmblircxrodpsixiocgxejoxjsbkucsumyyeomwxxsbrzppdnqchbtfbwudmahcnwmuumcfinnyprylxzfdqgpokkbflrfnenemclmkdskteeywdqesflbslyrfwhevlcweiuiulylzgaimhmcfgshnahtinhxzaynojpzfnebyvfhcsegkqglmpatvevazqrxdyvbaoscmlzfqfvcdnsdpvtoyyoagsjxvshxkzlgqloevlgckzrldegljphfwgvjqensvckwsnztkefirgxbjbzvhizfkiybjeykdjcyirydafirkqdzrtwtninktbnqstffrtlcvcpgmefhoigtetjtbmrrypibdrnhbcpjxoctptzjezyupjxpkozmqzumodxersrsxeglmofcupsnsevpmhtwmlpfzuvnzymlwenoytazcuvpcrttgxwdtjlclgoczjjqubptwtvwvmtybefecdaewzguprvuieszvhrybovlikraljgcrjlzdehhiybhmxdlgndxaqzeyynnptqljxpkvacjkjwsxagkinxzcuxgxuranqmtnrddxfjoobdybizwqrnigirgsvmojqewwzbhjkmchlnngotlshnoxglsiumllvhxacarllpozmkntvctvkykwrurwfcsiyljgjhxstksalimmhkpofqrsizedktdzeaatexyukimofzttutcozhpvvuvgymseswrvxfeqckeimzmqmiqdeyhaxhcoupzlgxiehexymtpttivvblgmqtbkzmwkpxdtxnvdehzesnvhzlgiqtjgkfflbtkzmvobpfpwbaxlpmdmlxdrvtnigiigtvtqxwgrhnbaihowodaqshxgkkxbcdywwuilblsfvlbjgvyjkzgpmawpntmkmfqlgvukoxjwpdjqxjwgyuyutvayqcwgippluwjctwjbxlphqhdjotmljrbinyjyclbozdrouozszxdvrtmkuyzcwunfldwoqqtwxsmlueatuyvnpqfpbuirgkpgscbaevefnjumtbabwzxdrdynbyvzpvjrspzucjbraoaeevapmbjttictbqqhnlunxhgvdwqqvonuyekqqljocdwjxcnyyjidzcygpiobjkknndlsrmijtwdoaomwhbigybwglzxwqjteijlugieyfqtowzzaylqnftedvadoyaegwrcfbxbwcadecpnqzpaecwruqrldongaqykyecvnqxxogvfkfceeocmmgxicawpalnrnwohnindbfjdskgtenmmcowomttbexjdqecyhafdvqplscdncqpeomkjqcucsaizjgbfhzwdkdmgasscnpjhkyhrsidhtykjdimhojnentamyhpwlhnzwgnbkcnmotznqnvfgalzbrklddlbyvbqahsbeikpfoonbwgwcnaawbhrzgvmzyvapzcdqmxmzhhjfaewdqwgxocrxowuhmhyrqkkhonkraodnayqdopapirbduzzhotexndktksmdnpjhmteghucubecpelqfljdvrspdxbjwdsrcwcmtyxshowektrhqzzbovhthlkvdgwrjemeihbiyrqznpkcfdacbnnkqqfxeehqnqcimoszrhmefyqnuppmyytaugirjsztoaqgmzqywhdgwhcaguvgopphnuvuccamfpcdshufemrfremjzbgslrevfjkprymjmfbdmzfntlfoxxhgkttlhtstnwsmloljpkvpyeggtowsoidlgsphfpaqpbqfrbmkyiaydgitorokwpcbupcynejsvaafmqlgnkvpewkaxdakrsdwwanycjorxbvvvrlghzmcvvqtfzbccwciahnfgslxjplstjxmxehsshavkzbdctcbrdqgmsfovxuqvntitvckajznlfsjviwekiawuqlscujvkucbxabzzelpfhadpyoibhaztzvampyuigherajzhxylchxjjmspijsqltdkrywgppxqgysldopxajrtqajchvmvecanhaggcmjlzxzykfhclvqmblircxrodpsixiocgxejoxjsbkucsumyyeomwxxsbrzppdnqchbtfbwudmahcnwmuumcfinnyprylxzfdqgpokkbflrfnenemclmkdskteeywdqesflbslyrfwhevlcweiuiulylzgaimhmcfgshnahtinhxzaynojpzfnebyvfhcsegkqglmpatvevazqrxdyvbaoscmlzfqfvcdnsdpvtoyyoagsjxvshxkzlgqloevlgckzrldegljphfwgvjqensvckwsnztkefirgxbjbzvhizfkiybjeykdjcyirydafirkqdzrtwtninktbnqstffrtlcvcpgmefhoigtetjtbmrrypibdrnhbcpjxoctptzjezyupjxpkozmqzumodxersrsxeglmofcupsnsevpmhtwmlpfzuvnzymlwenoytazcuvpcrttgxwdtjlclgoczjjqubptwtvwvmtybefecdaewzguprvuieszvhrybovlikraljgcrjlzdehhiybhmxdlgndxaqzeyynnptqljxpkvacjkjwsxagkinxzcuxgxuranqmtnrddxfjoobdybizwqrnigirgsvmojqewwzbhjkmchlnngotlshnoxglsiumllvhxacarllpozmkntvctvkykwrurwfcsiyljgjhxstksalimmhkpofqrsizedktdzeaatexyukimofzttutcozhpvvuvgymseswrvxfeqckeimzmqmiqdeyhaxhcoupzlgxiehexymtpttivvblgmqtbkzmwkpxdtxnvdehzesnvhzlgiqtjgkfflbtkzmvobpfpwbaxlpmdmlxdrvtnigiigtvtqxwgrhnbaihowodaqshxgkkxbcdywwuilblsfvlbjgvyjkzgpmawpntmkmfqlgvukoxjwpdjqxjwgyuyutvayqcwgippluwjctwjbxlphqhdjotmljrbinyjyclbozdrouozszxdvrtmkuyzcwunfldwoqqtwxsmlueatuyvnpqfpbuirgkpgscbaevefnjumtbabwzxdrdynbyvzpvjrspzucjbraoaeevapmbjttictbqqhnlunxhgvdwqqvonuyekqqljocdwjxcnyyjidzcygpiobjkknndlsrmijtwdoaomwhbigybwglzxwqjteijlugieyfqtowzzaylqnftedvadoyaegwrcfbxbwcadecpnqzpaecwruqrldongaqykyecvnqxxogvfkfceeocmmgxicawpalnrnwohnindbfjdskgtenmmcowomttbexjdqecyhafdvqplscdncqpeomkjqcucs";

    int nw = 1 << 8;  // copy 1MB to wbuf
    char* wbuf = calloc(nw, BLOCK_SIZE);
    for (i = 0; i < nw; ++i) {
        strcpy(wbuf + i * BLOCK_SIZE, data);
    }

    fs_write(fd[0], wbuf, 1 << 20);  // write 1MB

    // fs_lseek(fd[0], 0); // start at beginning

    // fs_write(fd[0], data, 100);  // write bytes 500-600

    // fs_write(fd[0], data, BLOCK_SIZE);

    /* SEEK TESTS */
    // fs_lseek(fd[0], 1000);

    /* READ TESTS */
    fs_lseek(fd[0], 0);  // seek to beginning

    int nbyte = 1 << 20;
    char* rbuf = calloc(1, nbyte);
    int rbyte = fs_read(fd[0], rbuf, nbyte);  // copy to buffer and read
    for (i = 0; i < nw * 2; ++i) {
        printf("fs_read @ byte %d:\t%.8s\n", i * BLOCK_SIZE / 2, rbuf + i * BLOCK_SIZE / 2);  // print output at every half block
        printf("\texpected:%.8s\n", wbuf + i * BLOCK_SIZE / 2);
    }

    printf("TOTAL BYTES READ: %d", rbyte);

    /* LIST TEST */
    char*** filenames = calloc(1, sizeof(char**));
    fs_listfiles(filenames);
    printf("Files on disk %s:\n", disk);
    char** temp = (*filenames);
    while (*temp != NULL) {
        printf("\t%s\n", *temp);
        ++temp;
    }

    /* FILESIZE TEST */
    for (i = 0; i < 2; ++i)
        if ((*filenames)[i] != NULL)
            printf("Size of %s is %d\n", (*filenames)[i], fs_get_filesize(fd[i]));

    /* DELETE TEST */
    // fs_delete(files[0]);
    // fs_delete(files[1]);

    fs_close(fd[0]);
    fs_close(fd[1]);

    umount_fs(disk);

    return 0;
}