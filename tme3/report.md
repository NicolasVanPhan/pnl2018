
Phan
Nicolas
PNL 2018
Report for TME3

Exercice 1
--------------------------------------------------------------------------------

#### Question 1

When lanching the VM with :
```sh
qemu-system-x86_64 -hda pnl-tp.img
```
The system starts but fails at mounting the second drive (hdb) which is normal because we didn't specify any image to be associated with hdb.

#### Question 2

Done

#### Question 3

Done


Exercice 2
--------------------------------------------------------------------------------

#### Question 1

Done

#### Question 2

A priori, one has n cores, one gives `make -j n`.
But actually `-j` specifies the number of tasks, and we could put a number > nb of cores.

#### Question 3

Done, well `file` returns that it's a linux image...

#### Question 4

There's a kernel in the `pnl-tp.img` image, so how can we know whether we're running on this kernel or on the new kernel we compiled ourselves?
Well, if the two versions are different it's good, but what if they are the same... A good way to ensure you're running on the good kernel is to give a little suffix to all kernels... See Question 5.

#### Question 5

Done

#### Question 6

There's no module loaded. Well the configuration we put is minimal, we didn't specify any module to be loaded at boot so... there's no module loaded after the boot.


Exercice 3
--------------------------------------------------------------------------------

#### Question 1

Done

#### Question 2

Well we add `init=hello` to the cmdline options aaaaannndd.....
Done

#### Question 3

At boot, we eventually see the `Hello World!` message and the 5 seconds pause,
then the system crashes on a kernel panic, specifying that we attempted to kill init.
Well init is a special process that should stay alive during the whole time the computer is on, if you kill something that should always stay alive, then of course the system will crash.
The init process is killed because the code it executed eventually terminated, so the init code should never terminate.

#### Question 4

Done, that's niiiice
Well, what `ps` does is reading stuffs in the `/proc` folder, but the `/proc` folder is mounted by the original code of init.
But since we replaced the original code, everything it had to do has not been done, a fortiori mounting `/proc` has not been done, hence the error :

```sh
Error, do this: mount -t proc proc /proc
```

#### Question 5

Well, fixing the problem only consists in executing the above command...
And then `ps` works fine.

#### Question 6

Executing `init` doesn't work because it's gonna launch the init code in a process with pid other than 1, to execute another program but keeping the same pid, use `exec`, so : `exec init` works.

??? How to google `Trying to run as user instance...`

Exercice 4
--------------------------------------------------------------------------------

#### Question 1

Done, that's cute, and yes there's an ash script called init at the root.

#### Question 2

The `-static` option prevents the compiler from making links (using names that will be resolved by including an externam dynamic library).

??? Why is it necessary ???
An initramfs is used to mount the root filesystem, but the shared libaries are located in the filesystem, so at the time the initramfs is being used, the root filesystem and hence the shared libraries are not mounted yet! So if our code contains links to shared libraries, these links won't be resolved and the code will fail.

#### Question 3

Done

#### Question 4

We get the same behaviour as first time :
 - `"Hello World!"` is displayed
 - The systems stands still for 5 seconds
 - The system crashes because attempted to kill init

Niiice

