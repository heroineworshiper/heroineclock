#include <stdio.h>
#include <stdint.h>
#include <math.h>

// gcc -o temp_tester temp_tester.c -lm;temp_tester

// radio shack thermistor
#define USE_RADIO_SHACK

// LITTELFUSE DC103G9G
//#define USE_LITTELFUSE


#define ADC_MAX (1023 << 6)

typedef struct
{
    int t;   // C * 10
    int r;  // kohms from the datasheet
} table_t;

// radio shack thermistor
table_t temp_table[] =
{
    { -200,  67760 },
    { -150,  53390 },
    { -100,  42450 },
    { -50,   33890 },
    { 0,	 27280 },
    { 50,	 22050 },
    { 100,	 17960 },
    { 150,	 14680 },
    { 200,	 12090 },
    { 250,	 10000 },
    { 300,	 8313  },
    { 350,	 6941  },
    { 400,	 5826  },
    { 450,	 4912  },
    { 500,	 4161  },
    { 550,	 3537  },
    { 600,	 3021  },
    { 650,	 2589  },
    { 700,	 2229  },
    { 750,	 1924  },
    { 800,	 1669  },
    { 850,	 1451  },
    { 900,	 1366  },
    { 950,	 1108  },
    { 1000,  938   },
    { 1050,  858   },
    { 1100,  758   }
};

#define TABLE_SIZE (sizeof(temp_table) / sizeof(table_t))





void main()
{
    int i, j, t = 0;

    int test_readings[] = 
    {
        59190,
        51000,
        41850,
        13119,
        12125
    };
    int total_test_readings = sizeof(test_readings) / sizeof(int);


// recompute resistances using B value
#ifdef USE_LITTELFUSE
// r in ohms
    float R0 = 10000;
// temp in kelvin
    float T0 = 25.0 + 273.15;
    float B = 3575;
    for(i = 0; i < TABLE_SIZE; i++)
    {
// temp in kelvin
        float T = (float)temp_table[i].t / 10 + 273.15;
        temp_table[i].r = (int)(R0 * exp(B * (1.0 / T - 1 / T0)));
    }
    
    printf("Using LITTELFUSE DC103G9G\n");
#else // USE_LITTELFUSE
    printf("Using RADIO SHACK\n");
#endif // !USE_LITTELFUSE

    int adc_table[TABLE_SIZE];
// series resistance in the circuit
    int series_r = 5000;
    
    printf("T * 10, ADC\n");
    for(i = 0; i < TABLE_SIZE; i++)
    {
        adc_table[i] = (int64_t)temp_table[i].r * 
            ADC_MAX / 
            (temp_table[i].r + series_r);
        printf("\t{ %d, %d },\n", temp_table[i].t, adc_table[i]);
    }
    
    printf("Test readings ADC = Temp\n");
    for(i = 0; i < total_test_readings; i++)
    {
        int adc = test_readings[i];
        
        for(j = 0; j < TABLE_SIZE; j++)
        {
            if(adc_table[j] < adc)
            {
                int32_t t_high = temp_table[j].t;
                int32_t adc_high = adc_table[j - 1];

                int32_t t_low = temp_table[j - 1].t;
                int32_t adc_low = adc_table[j];

                t = t_high - (adc - adc_low) * (t_high - t_low) / (adc_high - adc_low);
                break;
            }
        }
        
        printf("%d = %f\n", adc, (float)t / 10);
    }
}






