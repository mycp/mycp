
static const unsigned int primes[] =
{
  11,
  19,
  37,
  73,
  109,
  163,
  251,
  367,
  557,
  823,
  1237,
  1861,
  2777,
  4177,
  6247,
  9371,
  14057,
  21089,
  31627,
  47431,
  71143,
  106721,
  160073,
  240101,
  360163,
  540217,
  810343,
  1215497,
  1823231,
  2734867,
  4102283,
  6153409,
  9230113,
  13845163,
};

static const unsigned int nprimes = sizeof (primes) / sizeof (primes[0]);

unsigned int
spaced_primes_closest (unsigned int num)
{
  unsigned int i;

  for (i = 0; i < nprimes; i++)
    if (primes[i] > num)
      return primes[i];

  return primes[nprimes - 1];
}
