## Runtime (`process` call only)

We run 10 experiments for each convolver settings. We test time domain FIR convolver, frequency domain FFT convolver with block size 256, 512, 1024, 2048, 4096, 8192, 16384, and 32768. We use `std::chrono::steady_clock` to evaluate runtime more accurately than `ctime`. The runtime unit is nanosecond (1e-9s).

We run all 90 experiments on the same input file (1028361 samples) and IR file (214016 samples) on the same computer with no other system load.

The experiment raw data is listed below:

| BlockSize | Exp 0        | Exp 1        | Exp 2        | Exp 3        | Exp 4        | Exp 5        | Exp 6        | Exp 7        | Exp 8        | Exp 9        | AVG         |
| --------- | ------------ | ------------ | ------------ | ------------ | ------------ | ------------ | ------------ | ------------ | ------------ | ------------ | ----------- |
| Time      | 534092818800 | 507392937400 | 604477801600 | 704698770800 | 676857390700 | 525647673400 | 578527688000 | 479702965400 | 495317900700 | 472913106100 | 557962905290 |
| 256       | 26255107400  | 30781267300  | 32633326000  | 28449182000  | 27971667900  | 31935829400  | 28741947700  | 27076440300  | 28329957800  | 28183738000  | 29035846380 |
| 512       | 13867580100  | 16921592200  | 15044385800  | 15427789100  | 13691386700  | 14185895100  | 18670749100  | 15373475500  | 13967057400  | 14501443000  | 15165135400 |
| 1024      | 7673700800   | 7504299600   | 7398391300   | 7684410900   | 7437939100   | 7726721800   | 7759071000   | 7714008600   | 9872602700   | 8952319000   | 7972346480  |
| 2048      | 4319090400   | 3948207400   | 4028238400   | 4029799200   | 4024757200   | 3960182300   | 3938185600   | 3877580700   | 3905494300   | 3851614200   | 3988314970  |
| 4096      | 2209569200   | 2090650100   | 2121871200   | 2127310900   | 2033298700   | 2061201300   | 2123743400   | 2099357600   | 2091577600   | 2175055200   | 2113363520  |
| 8192      | 1421631800   | 1556480600   | 1494852300   | 1446110700   | 1452868800   | 1443496600   | 1545761700   | 1526350200   | 1510624900   | 1439304700   | 1483748230  |
| 16384     | 981889700    | 926660100    | 922656800    | 974655500    | 986397000    | 1003178500   | 921550800    | 881640000    | 918724000    | 858121700    | 937547410   |
| 32768     | 558893600    | 563068700    | 538381400    | 542289200    | 564541000    | 584424500    | 538133400    | 579705200    | 646199500    | 549394900    | 566503140   |

And the visualized figure for more intuitive comparison:

![fig](fig.png)

Notice that the y-axis is $\log_2$ scale. Compared with time domain convolution, the fastest FFT convolver is ~984.9246x faster.

## Some observations

When FFT block size axis and runtime axis are both under $\log_2$ scale, the relationship between runtime and block size seems to be linear. That is to say, doubling block size approximately halves the runtime.

If we perform linear regression on FFT part data, we have

![fig2](fig2.png)

The exponential part ($e^{-0.56136x} \approx 0.57043^x$) clearly supports the previous observation that doubling block size approximately halves (actually approximately `*= 0.57043`) the runtime. This also shows the ratio of constant in time complexity. When FFT block size ~= $5.553613770329971$, the FFT convolver should have similar runtime performance with the time domain convolver.

When FFT block size = 1, it will take ~2.2867x time of time domain convolver to finish convolution. This reflects FFT's large constant $c$ in $\mathcal{O}(c n \log n)$ compared to time domain convolver.
