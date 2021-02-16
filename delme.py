xxx = False
with open("license.txt", "r") as filex:
    for i, l in enumerate(filex):
        print(i, l)

        if (i > 3 and not xxx):
            for h in range(0,10):
                a=filex.readline()
                print("X", a)

            xxx=True
