// 1-init
#include "../includes/philosophers.h"

int	is_someone_dead(t_data *data)
{
	pthread_mutex_lock(&(data->dead));
	if (data->someone_died)
	{
		pthread_mutex_unlock(&(data->dead));
		return (1);
	}
	pthread_mutex_unlock(&(data->dead));
	return (0);
}

int	is_someone_finished(t_data *data)
{
	pthread_mutex_lock(&(data->finished));
	if (data->someone_finished)
	{
		pthread_mutex_unlock(&(data->finished));
		return (1);
	}
	pthread_mutex_unlock(&(data->finished));
	return (0);
}

void	set_someone_finished(t_data *data)
{
	pthread_mutex_lock(&(data->finished));
	data->someone_finished = 1;
	pthread_mutex_unlock(&(data->finished));
}

int	eat_enough(t_data *data)
{
	int		i;
	int		count;

	count = 0;
	i = 0;
	if (!data->n_eat)
		return (0);
	while (i < data->n_phil)
	{
		pthread_mutex_lock(&(data->philo[i].eating_mtx));
		if (data->n_eat <= data->philo[i].eat_counter)
			count++;
		pthread_mutex_unlock(&(data->philo[i].eating_mtx));
		i++;
	}
	if (i <= count)
		return (1);
	return (0);
}

void	*check_status(void *args)
{
	t_data	*data;
	int		i;

	data = (t_data *)args;
	while (!is_someone_finished(data) && !is_someone_dead(data))
	{
		i = 0;
		while (i < data->n_phil)
		{
			pthread_mutex_lock(&(data->philo[i].eating_mtx));
			if (current_time() - data->philo[i].last_eat >= data->t_die)
			{
				pthread_mutex_lock(&(data->mtx_someone_died));
				print(data->philo, " died");
				data->someone_died = 1;
				pthread_mutex_unlock(&(data->mtx_someone_died));
				pthread_mutex_unlock(&(data->philo[i].eating_mtx));
				break ;
			}
			pthread_mutex_unlock(&(data->philo[i++].eating_mtx));
			if (eat_enough(data))
				set_someone_finished(data);
		}
	}
	return (exit_code(data));
}




// 2-philo
void	start_thread(t_data *data)
{
	int				i;
	pthread_t		check;

	i = 0;
	data->someone_died = 0;
	data->someone_finished = 0;
	while (i < data->n_phil)
	{
		data->philo[i].last_eat = current_time();
		if (pthread_create(&(data->philo[i].tid), NULL,
				routin, &(data->philo[i])) != 0)
		{
			perror("pthread_create");
			return ;
		}
		i++;
	}
	pthread_create(&check, NULL, check_status, data);
	i = 0;
	while (i < data->n_phil)
		pthread_join(data->philo[i++].tid, NULL);
	pthread_join(check, NULL);
}

pthread_mutex_t	*initialize_philo(t_data *data)
{
	int				i;
	pthread_mutex_t	*fork_array;

	i = 0;
	fork_array = malloc(sizeof(pthread_mutex_t) * data->n_phil);
	if (!fork_array)
		perror("Memory allocation failed");
	while (i < data->n_phil)
	{
		data->philo[i].n = i + 1;
		data->philo[i].eat_counter = 0;
		data->philo[i].data = data;
		data->philo[i].fork_right = &fork_array[i];
		pthread_mutex_init(&fork_array[i], NULL);
		pthread_mutex_init(&(data->philo[i].eating_mtx), NULL);
		if (i == data->n_phil - 1)
			data->philo[0].fork_left = &fork_array[i];
		else
			data->philo[i + 1].fork_left = &fork_array[i];
		i++;
	}
	data->t_start = current_time();
	start_thread(data);
	return (fork_array);
}

void	free_datas(pthread_mutex_t *forks, t_data *data, t_philo *philo)
{
	free(forks);
	free(data);
	free(philo);
}

void	parse_variable(char **argv)
{
	t_data			*data;
	pthread_mutex_t	*forks;

	data = malloc(sizeof(t_data));
	if (!data)
		perror("Memory allocation failed");
	pthread_mutex_init(&data->print, NULL);
	pthread_mutex_init(&data->dead, NULL);
	pthread_mutex_init(&data->stop, NULL);
	pthread_mutex_init(&data->finished, NULL);
	pthread_mutex_init(&data->mtx_someone_died, NULL);
	data->n_phil = ft_atoi(argv[1]);
	data->t_die = ft_atoi(argv[2]);
	data->t_eat = ft_atoi(argv[3]);
	data->t_sleep = ft_atoi(argv[4]);
	data->n_eat = 0;
	data->philo = malloc(sizeof(t_philo) * data->n_phil);
	if (!data->philo)
		perror("Memory allocation failed");
	if (argv[5])
		data->n_eat = ft_atoi(argv[5]);
	if (argv[5] && data->n_eat == 0)
		return ;
	forks = initialize_philo(data);
	free_datas(forks, data, data->philo);
}

int	main(int argc, char **argv)
{
	if (argc != 5 && argc != 6)
		return (1);
	if (!is_valid_integer(argv))
		return (1);
	parse_variable(argv);
	return (0);
}

//3-print
long long	current_time(void)
{
	struct timeval	current;

	gettimeofday(&current, NULL);
	return (current.tv_sec * 1000 + current.tv_usec / 1000);
}

void	print(t_philo *philo, char *str)
{
	pthread_mutex_lock(&(philo->data->dead));
	if (!philo->data->someone_died && !eat_enough(philo->data))
	{
		pthread_mutex_lock(&(philo->data->print));
		printf("%lld %d %s\n", current_time() - philo->data->t_start,
			philo->n, str);
		pthread_mutex_unlock(&(philo->data->print));
	}
	pthread_mutex_unlock(&(philo->data->dead));
}

void	pause_time(int t)
{
	long int	time;

	time = current_time();
	while (current_time() - time < t)
		usleep(t / 10);
}


//4- routin

int	test_(t_philo *philo)
{
	if (!philo->data->n_eat)
		return (0);
	pthread_mutex_lock(&(philo->eating_mtx));
	if (philo->eat_counter == philo->data->n_eat)
	{
		pthread_mutex_unlock(&(philo->eating_mtx));
		return (1);
	}
	pthread_mutex_unlock(&(philo->eating_mtx));
	return (0);
}

void	eating(t_philo *philo)
{
	if (!test_(philo))
	{
		print(philo, " is eating");
		philo->eat_counter++;
		pthread_mutex_lock(&(philo->eating_mtx));
		philo->last_eat = current_time();
		pthread_mutex_unlock(&(philo->eating_mtx));
		pause_time(philo->data->t_eat);
		pthread_mutex_unlock(philo->fork_left);
		pthread_mutex_unlock(philo->fork_right);
	}
}

void	taking_fork(t_philo *philo)
{
	pthread_mutex_lock(philo->fork_right);
	print(philo, " has taken a fork");
	pthread_mutex_lock(philo->fork_left);
	print(philo, " has taken a fork");
}

void	*routin(void *arg)
{
	t_philo		*philo;

	philo = (t_philo *)arg;
	if ((philo->n + 1) % 2)
		pause_time(philo->data->t_eat / 10);
	while (!eat_enough(philo->data) && !is_someone_finished(philo->data)
		&& !is_someone_dead(philo->data))
	{
		taking_fork(philo);
		eating(philo);
		if (!eat_enough(philo->data) && !is_someone_finished(philo->data))
		{
			print(philo, " is sleeping");
			pause_time(philo->data->t_sleep);
		}
		if (!eat_enough(philo->data) && !is_someone_finished(philo->data))
			print(philo, " is thinking");
	}
	return (NULL);
}

//5-utils
int	ft_isspace(int c)
{
	return ((c >= 9 && c <= 13) || c == 32);
}

int	is_digit(char *args)
{
	int	i;

	i = 0;
	if ((args[i] == '+') && ft_isdigit(args[i + 1]))
		i++;
	while (args[i])
	{
		while (args[i] == ' ' || args[i] == '\t')
			i++;
		if ((args[i] == '+') && !(args[i - 1]))
			i++;
		if (!ft_isdigit(args[i]))
			return (0);
		i++;
	}
	return (1);
}

int	is_valid_integer(char **args)
{
	int		i;

	i = 2;
	while (args[i])
	{
		if (!is_digit(args[i]))
			return (0);
		i++;
	}
	return (1);
}

long	long_atoi(char *str)
{
	int			i;
	int			flag;
	long		num;

	i = 0;
	flag = 1;
	num = 0;
	while (ft_isspace(str[i]))
		i++;
	if (str[i] == '+' || str[i] == '-')
	{
		if (str[i] == '-')
			flag *= -1;
		i++;
	}
	while (ft_isdigit(str[i]))
	{
		num = num * 10 + (str[i] - '0');
		i++;
	}
	return (flag * num);
}

void	*exit_code(t_data *data)
{
	int		i;

	i = 0;
	while (i < data->n_phil)
		pthread_mutex_unlock(data->philo[i++].fork_right);
	return (NULL);
}
