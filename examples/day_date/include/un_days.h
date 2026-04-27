#ifndef UN_DAYS_H
#define UN_DAYS_H

struct UNEvent {
    int month;
    int day;
    const char* summary;
};

// Fixed date events from your list
const UNEvent un_events[] = {
    {1, 27, "International Day of Commemoration in Memory of the Victims of the Holocaust"},
    {2, 4, "World Cancer Day"},
    {2, 6, "International Day of Zero Tolerance to Female Genital Mutilation"},
    {2, 11, "International Day of Women and Girls in Science"},
    {2, 13, "World Radio Day"},
    {2, 20, "World Day of Social Justice"},
    {2, 21, "International Mother Language Day"},
    {3, 1, "Zero Discrimination Day"},
    {3, 3, "World Wildlife Day"},
    {3, 8, "International Women's Day"},
    {3, 20, "International Day of Happiness"},
    {3, 20, "French Language Day"},
    {3, 21, "World Poetry Day"},
    {3, 21, "International Day of Nowruz"},
    {3, 21, "World Down Syndrome Day"},
    {3, 21, "International Day of Forests"},
    {3, 22, "World Water Day"},
    {3, 23, "World Meteorological Day"},
    {3, 24, "World Tuberculosis Day"},
    {3, 24, "International Day for the Right to the Truth"},
    {3, 25, "International Day of Remembrance of the Victims of Slavery"},
    {3, 25, "International Day of Solidarity with Detained Staff"},
    {4, 2, "World Autism Awareness Day"},
    {4, 4, "International Day for Mine Awareness"},
    {4, 6, "International Day of Sport for Development and Peace"},
    {4, 7, "World Health Day"},
    {4, 12, "International Day of Human Space Flight"},
    {4, 20, "Chinese Language Day"},
    {4, 21, "World Creativity and Innovation Day"},
    {4, 22, "International Mother Earth Day"},
    {4, 23, "English Language Day"},
    {4, 23, "World Book and Copyright Day"},
    {4, 25, "World Malaria Day"},
    {4, 26, "World Intellectual Property Day"},
    {4, 28, "World Day for Safety and Health at Work"},
    {4, 30, "International Jazz Day"},
    {5, 2, "World Tuna Day"},
    {5, 3, "World Press Freedom Day"},
    {5, 15, "International Day of Families"},
    {5, 17, "World Telecommunication Day"},
    {5, 21, "World Day for Cultural Diversity"},
    {5, 22, "International Day for Biological Diversity"},
    {5, 29, "International Day of UN Peacekeepers"},
    {5, 31, "World No-Tobacco Day"},
    {6, 1, "Global Day of Parents"},
    {6, 4, "International Day of Innocent Children Victims of Aggression"},
    {6, 5, "World Environment Day"},
    {6, 6, "Russian Language Day"},
    {6, 8, "World Oceans Day"},
    {6, 12, "World Day Against Child Labour"},
    {6, 14, "World Blood Donor Day"},
    {6, 20, "World Refugee Day"},
    {6, 21, "International Day of Yoga"},
    {6, 26, "International Day against Drug Abuse"},
    {7, 11, "World Population Day"},
    {7, 18, "Nelson Mandela International Day"},
    {7, 30, "International Day of Friendship"},
    {8, 12, "International Youth Day"},
    {8, 19, "World Humanitarian Day"},
    {9, 8, "International Literacy Day"},
    {9, 15, "International Day of Democracy"},
    {9, 21, "International Day of Peace"},
    {9, 27, "World Tourism Day"},
    {9, 30, "International Translation Day"},
    {10, 1, "International Day of Older Persons"},
    {10, 2, "International Day of Non-Violence"},
    {10, 5, "World Teachers' Day"},
    {10, 16, "World Food Day"},
    {10, 24, "United Nations Day"},
    {11, 14, "World Diabetes Day"},
    {11, 16, "International Day for Tolerance"},
    {11, 20, "Universal Children's Day"},
    {11, 21, "World Television Day"},
    {12, 1, "World AIDS Day"},
    {12, 10, "Human Rights Day"},
    {12, 18, "Arabic Language Day"}
};

const char* getUNEvent(int m, int d) {
    for (int i = 0; i < sizeof(un_events)/sizeof(un_events[0]); i++) {
        if (un_events[i].month == m && un_events[i].day == d) {
            return un_events[i].summary;
        }
    }
    return NULL;
}

#endif
