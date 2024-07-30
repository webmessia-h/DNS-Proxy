import time
from threading import Timer

import dns.resolver


def send_query(resolver, domain):
    try:
        resolver.query(domain)
    except Exception as e:
        print(f"Query failed for {domain}: {e}")


def load_test(target_ip, target_port, queries_per_second, duration, domains):
    resolver = dns.resolver.Resolver()
    resolver.nameservers = [target_ip]
    resolver.port = target_port

    interval = 1.0 / queries_per_second
    end_time = time.time() + duration

    def perform_queries():
        if time.time() < end_time:
            for domain in domains:
                send_query(resolver, domain)
            Timer(interval, perform_queries).start()

    perform_queries()


if __name__ == "__main__":
    target_ip = "127.0.0.1"
    target_port = 53
    queries_per_second = 10000
    duration = 60  # seconds
    domains = [
        "microsoft.com",
        "google.com",
        "github.com",
    ]  # Add more domains as needed

    load_test(target_ip, target_port, queries_per_second, duration, domains)
